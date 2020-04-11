#include "RoutingProtocolImpl.h"
#include <string.h>
#include "DV_Protocol.h"
#include "LS_Protocol.h"
#include "limits.h"
#include <algorithm>

#define PING_PACK_SIZE 12
#define PONG_PACK_SIZE 12

// init node
RoutingProtocolImpl::RoutingProtocolImpl(Node *n) : RoutingProtocol(n) {
  sys = n;
}

// deconstructor clear linkmap and dv or ls
RoutingProtocolImpl::~RoutingProtocolImpl() {
  // add your own code (if needed)
    unordered_map<unsigned short, LinkTable>::iterator it = linkmap.begin();
    while (it != linkmap.end()) {
            //LinkTable lnk = it->second;
            linkmap.erase(it++);
    }
    if(this->protocol == P_LS){
        free(ls);
    }else {
        free(dv);
    }
}

// set the alarm
void RoutingProtocolImpl::setAlarmType( RoutingProtocol *r, unsigned int duration, void *d){
    sys->set_alarm(r,duration,d);
}

void RoutingProtocolImpl::init(unsigned short num_ports, unsigned short router_id, eProtocolType protocol_type) {
  // sanity check for protocol
  if( protocol_type != P_LS && protocol_type != P_DV) {
    fprintf(stderr, "Incorrect protocol initialization\n");
    exit(EXIT_FAILURE); 
  }
  this->num_ports = num_ports;
  this->router_id = router_id;
  this->protocol = protocol_type; //enum defined in global.h - imported in RoutingProtocol.h
  setAlarmType(this, 0, (void*)this->ping); //initialize ping
  setAlarmType(this, checkalarm, (void*)this->update); 
  switch(this->protocol){//protocol initialization and setAlarm
    case P_LS:
        ls = new LS_Protocol(this->router_id);
        setAlarmType(this, lsalarm, (void*)this->linkstate);
        break;
    case P_DV:
        dv = static_cast<DV_Protocol*>(malloc(sizeof(DV_Protocol)));
        dv->setRouterID(this->router_id);
        setAlarmType(this, dvalarm, (void*)this->distancevector); 
        break;
  }
}
//handle all alarms
void RoutingProtocolImpl::handle_alarm(void *data) {
  char* alarmtype = reinterpret_cast<char*>(data);
  if (strcmp(alarmtype,this->ping) == 0) pingTime();
  else if(strcmp(alarmtype, this->update) == 0) updateTime();
  else if(strcmp(alarmtype, this->distancevector) == 0) dvTime();
  else if(strcmp(alarmtype, this->linkstate) == 0) lsTime();
  else {
    fprintf(stderr, "Not able to recognize %s alarm\n", alarmtype);
    exit(EXIT_FAILURE);
  }
}
//master recv that distributes it to other helper functions
void RoutingProtocolImpl::recv(unsigned short port, void *packet, unsigned short size) {
  // need to free recv after processing
  // need to check what kind of packet has been received - there are 5 different kinds of packets!
  char type = *(char*)packet; //first byte contains the packet type
  if(type == DATA) recvDataPacket((char*)packet,size);
  else if(type == PING) recvPingPacket(port,(char*) packet, size); 
  else if(type == PONG) recvPongPacket(port, (char*) packet);
  else if(type == LS) recvLSPacket(port, (char*) packet, size );
  else if(type == DV) recvDVPacket((char*) packet, size);
  else {
    fprintf(stderr, "Packet received has incorrect type\n");
    free(packet);
  }
}

//need to ping an reset alarm
void RoutingProtocolImpl::pingTime(){
    for(int i=0;i<this->num_ports;i++){
        sendPingPacket(i);
    }
    setAlarmType(this, pingalarm, (void*)this->ping); 
}
//send ls packet and reset alarm
void RoutingProtocolImpl::lsTime(){
    sendLSPacket();
    setAlarmType(this, lsalarm, (void*)this->linkstate);    
}
//send dv packet and reset alarm
void RoutingProtocolImpl::dvTime(){
    sendDVPacket();
    setAlarmType(this, dvalarm, (void*)this->distancevector);   
}
//send DV
void RoutingProtocolImpl::sendDVPacket()
{
    for (auto &link: linkmap)
    {
        unsigned short d_ID = link.first;
        auto ltable = link.second;
        unsigned short port_ID = ltable.port_ID;
        sendDVPacketHelper(port_ID, d_ID);
    }
}

//periodic check to see if any link is dead or alive
void RoutingProtocolImpl::updateTime(){
    bool ischanged = checkTopology();
    if (this->protocol == P_LS) {
            bool lschanged = ls->checkLinkState(sys->time());
            if(ischanged || lschanged){
                ls->shortestPath(routingtable);
                sendLSPacket();
            }   
    } else {
        if (ischanged && updateDVTable())
            sendDVPacket();
    }   
    setAlarmType(this, checkalarm, (void*)this->update);
}

//create and send PING
//leave packet + 6 empty in order to check PONG for PONG ID
//NOTE: enum casted to char takes only 1 byte but the next byte is reserved
void RoutingProtocolImpl::sendPingPacket(int port){
    char* ping_packet = (char*)malloc(PING_PACK_SIZE);
    *ping_packet = (char)PING;
    *(unsigned short*)(ping_packet + 2) = (unsigned short)htons(PING_PACK_SIZE);
    *(unsigned short*)(ping_packet + 4) = (unsigned short)htons(this->router_id);
    *(unsigned int*)(ping_packet + 8) = (unsigned int)htonl(sys->time());
    sys->send(port, ping_packet, PING_PACK_SIZE);
}
//send LS
void RoutingProtocolImpl::sendLSPacket(){
    /* flood packets to get global topology*/
    cout<<"sendLSPacket"<<endl;
    unsigned short ls_pack_size = 12 + (linkmap.size() * 4);
    for (unordered_map<unsigned short, LinkTable>::iterator it = linkmap.begin(); it != linkmap.end(); ++it) {
            char* ls_packet = (char*)malloc(ls_pack_size);
            ls->createLSPacket(ls_packet, ls_pack_size);
            sys->send(it->second.port_ID, ls_packet, ls_pack_size);
    }           
    ls->increment();
}

void RoutingProtocolImpl::sendDVPacketHelper(unsigned short port_ID, unsigned short d_ID){
    unsigned short num_entries = (unsigned short) dvtable.size();
    unsigned short size = 8 + num_entries * 4;
    char* dv_packet = (char*)malloc(size);
    // Write header
    *dv_packet = (char)DV;
    *(unsigned short*)(dv_packet + 2) = (unsigned short)htons(size);
    *(unsigned short*)(dv_packet + 4) = (unsigned short)htons(this->router_id);
    *(unsigned short*)(dv_packet + 6) = (unsigned short)htons(d_ID);
    // Write table (node ID - cost)
    int i = 0;
    for (auto line: dvtable)
    {
        unsigned short node_ID = line.first;
        auto cost_hop = line.second;
        unsigned short cost = cost_hop.first;
        unsigned short hop = cost_hop.second;
        if (d_ID == hop) // Poison reverse
        {
            cost = USHRT_MAX;
        }
        *(unsigned short*)(dv_packet + 8 + i*4) = (unsigned short)htons(node_ID);
        *(unsigned short*)(dv_packet + 10 + i*4) = (unsigned short)htons(cost);
        i += 1;
    }   
    sys->send(port_ID, dv_packet, size);
}

void RoutingProtocolImpl::recvDataPacket(char* packet, unsigned short size){
    //this is the end point?
    unsigned short s_ID = (unsigned short)ntohs(*(unsigned short*)(packet+6));
    //packet intended for this router
    if(this->router_id == s_ID){
        free(packet);
        return;
    }
    //update table
    updateTable(s_ID, packet, size);
}

void RoutingProtocolImpl::recvPingPacket(unsigned short port, char* packet, unsigned short size){
    //need to send back a PONG to the  router that sent the PING
    unsigned short packet_size = (unsigned short)ntohs(*(unsigned short*)(packet + 2));
    if(packet_size != PING_PACK_SIZE){
        free(packet);
        return;
    }//checked for size
    unsigned short recv_id = (unsigned short)ntohs(*(unsigned short*)(packet + 4));
    unsigned int sendtime = (unsigned int)ntohl(*(unsigned int*)(packet+8));       
    char* pong_packet = (char*)malloc(PONG_PACK_SIZE);
    *pong_packet = (char)PONG;  
    *(unsigned short*)(pong_packet + 2) = (unsigned short)htons(size);
    *(unsigned short*)(pong_packet + 4) = (unsigned short)htons(this->router_id);
    *(unsigned short*)(pong_packet + 6) = (unsigned short)htons(recv_id);//add the corresponding id
    *(unsigned int*)(pong_packet + 8) = (unsigned int)htonl(sendtime);
    free(packet);//free packet
    sys->send(port, pong_packet, PONG_PACK_SIZE);
}

void RoutingProtocolImpl::recvPongPacket(unsigned short port, char* packet){
    //pong packet - check if this is the correct end point
    if(this->router_id != (unsigned short)ntohs(*(unsigned short*)(packet+6))){
        free(packet);
        return;
    }
    //calculate linkcost    
    unsigned int sendtime = (unsigned int)ntohl(*(unsigned int*)(packet + 8));
    unsigned short linkcost = (short)(sys->time() - sendtime);
    unsigned short s_ID = (unsigned short)ntohs(*(unsigned short*)(packet + 4));
    linkcosts.insert(pair<unsigned short, unsigned short>(s_ID, linkcost));
    //timeout
    unsigned int expire_timeout = sys->time() + pongto;
    unordered_map<unsigned short, LinkTable>::iterator it = linkmap.find(s_ID);
    if (it != linkmap.end()) {//update link table
            LinkTable lnk = it->second;
            lnk.expire_timeout = expire_timeout;
        linkmap[s_ID] = lnk;
    } else {//add to link table
        LinkTable lnk;  
        lnk.expire_timeout = expire_timeout;
            lnk.port_ID = port ;
            linkmap[s_ID] = lnk;
    }       
    switch(this->protocol){
        case P_LS:
            if(ls->updatePONG(s_ID, lsto, linkcost, sys->time())){
                    ls->shortestPath(routingtable);
                    sendLSPacket();
                }
            break;
        case P_DV:
            bool isDVTableChanged = updateDVTable(); // update the dvtable when linkcosts change detected by pong packet
            if (isDVTableChanged)
                sendDVPacket();
            break;
        
    }
    free(packet);       
}

// LS Packet format:
// 0      7|8       15|16        |        31|
//   type  | reserved |         size        |
//     source ID      |       dest ID       |
//      Node ID 1     |        cost 1       |
//      Node ID 2     |        cost 2       |
//      .......       |        .....        |
//
void RoutingProtocolImpl::recvLSPacket(unsigned short port, char* packet, unsigned short size){
    //check fidelity and then update by flooding
    if (ls->checkSeqNum(packet)) {
        ls->updateLS(packet, lsto, sys->time(), size);
        for (unordered_map<unsigned short, LinkTable>::iterator lit = linkmap.begin(); lit != linkmap.end(); ++lit) {
                if (port != lit->second.port_ID){
                    char* flood = (char*)malloc(size);
                    memcpy(flood, packet, size);
                    sys->send(lit->second.port_ID, flood, size);
                }
            }   
            ls->shortestPath(routingtable);
    }
    free(packet);
}

// DV Packet format:
// 0      7|8       15|16        |        31|
//   type  | reserved |         size        |
//     source ID      |       dest ID       |
//      Node ID 1     |        cost 1       |
//      Node ID 2     |        cost 2       |
//      .......       |        .....        |
//
// A: this->router_id
// V: s_ID (a neighbor node)
// Y: Node ID n
// D(V, Y)= node_cost
void RoutingProtocolImpl::recvDVPacket(char* packet, unsigned short size){
    bool isUpdated = false;
    // Read packet
    // unsigned short size = (unsigned short)ntohs(*(unsigned short*)(packet + 2)); // Is the param size the same as the size field in the packet??
    unsigned short s_ID = (unsigned short)ntohs(*(unsigned short*)(packet + 4));
    unsigned short d_ID = (unsigned short)ntohs(*(unsigned short*)(packet + 4));

    // if s_ID is not a neighbor, discard this packet
    auto lit = linkcosts.find(s_ID);
    if (lit == linkcosts.end())
    {
        free(packet);
        return;
    }

    // Read node_ID - cost_VY table
    vector<unsigned short> node_IDs = {};
    vector<unsigned short> cost_VYs = {};

    for (int i = 8; i < size; i += 4)
    {
        unsigned short node_ID = (unsigned short)ntohs(*(unsigned short*)(packet + i));
        unsigned short cost_VY = (unsigned short)ntohs(*(unsigned short*)(packet + i + 2));
        node_IDs.push_back(node_ID);
        cost_VYs.push_back(cost_VY);
    }

    // Check the table
    for (size_t i = 0; i < node_IDs.size(); i += 1)
    {
        unsigned short node_ID = node_IDs[i];
        unsigned short cost_VY = cost_VYs[i];

        // Query the local DV table
        // If V == A itself, it should be skipped
        if (node_ID == this->router_id)
            continue;
        // If D(V, Y) is infinite, it should be ignored
        if (cost_VY == USHRT_MAX)
            continue;
        auto it = dvtable.find(node_ID);
        if (it == dvtable.end())
        {
            // If A hasn't seen node Y before, add it to dvtable
            auto lit = linkcosts.find(s_ID);
            unsigned short cost_AV;
            if (lit == linkcosts.end())
                fprintf(stderr, "Neighbor Node does not exist\n");
            else
                cost_AV = lit->second;
            unsigned short cost_AYV = cost_AV + cost_VY;
            auto cost_hop = pair<unsigned short, unsigned short>(cost_AYV, s_ID);
            dvtable.insert(pair<unsigned short, pair<unsigned short, unsigned short>>(node_ID, cost_hop));
            isUpdated = true;
        }
        else
        {
            // d(A, Y) = d(A, V) + d(V, Y)
            auto &cost_hop = it->second;
            unsigned short cost_AY = cost_hop.first;
            unsigned short nexthop = cost_hop.second; // old V, may or may not be s_ID
            auto lit = linkcosts.find(s_ID);
            unsigned short cost_AV;
            if (lit == linkcosts.end())
                fprintf(stderr, "Neighbor Node does not exist\n");
            else
                cost_AV = lit->second;
            unsigned short cost_AYV = cost_AV + cost_VY; // Need a table to store neighbors and costs?
            if (cost_AYV < cost_AY){//when it's minimum
                cost_hop = pair<unsigned short, unsigned short>(cost_AYV, s_ID); // update dvtable for dest Y with new cost and new hop V
                isUpdated = true;
            }
            else if (cost_AYV > cost_AY && nexthop == s_ID){ // when the next hop is the same, but cost increases
                cost_hop = pair<unsigned short, unsigned short>(cost_AYV, s_ID);
                isUpdated = true;
            } // when the next hop is different and cost increases, nothing needs to be done
        }

    }

    // Any line in A's dvtable should be removed if it has a destination Y via hop V, 
    // and link_YV does not exist in the table received from V
    vector<unsigned short> toErase = {};
    for (auto &line: dvtable)
    {
        unsigned short dest_ID = line.first;
        auto cost_hop = line.second;
        unsigned short hop = cost_hop.second;
        if (dest_ID != hop && hop == s_ID) {
            if (std::find(node_IDs.begin(), node_IDs.end(), dest_ID) == node_IDs.end()) {
                toErase.push_back(dest_ID);
                isUpdated = true;
            }
        }
    }

    for (auto ID: toErase)
    {
        dvtable.erase(ID);
    }
    free(packet);


    if (isUpdated)
    {
        printDVTable();
        // update the routing table accordingly
        updateRoutingTableDV();
        sendDVPacket();
    }
}

// update dvtable, only used when the linkcosts is changed (either by PONG or linkdying/coming up)
bool RoutingProtocolImpl::updateDVTable()
{
    bool result = false;
    // Check if there're new lines in linkcosts table
    for (auto line: linkcosts)
    {
        unsigned short neighbor_node_ID = line.first;
        unsigned short cost = line.second;
        auto it = dvtable.find(neighbor_node_ID);
        if (it != dvtable.end())
        {
            auto cost_hop = it->second;
            unsigned short old_cost = cost_hop.first;
            if (old_cost <= cost)
                continue;
            dvtable.erase(neighbor_node_ID);
        }
        // update the dvtable entry if the entry doesn't exist, or if cost < old_cost 
        result = true;
        auto cost_hop = pair<unsigned short, unsigned short>(cost, neighbor_node_ID);
        dvtable.insert(pair<unsigned short, pair<unsigned short, unsigned short>>(neighbor_node_ID, cost_hop));
    }

    // Check if there're lines removed (linkdying)
    vector<unsigned short> toErase = {};
    for (auto line: dvtable)
    {
        unsigned short node_ID = line.first;
        auto cost_hop = line.second;
        unsigned short neighbor_node_ID = cost_hop.second;
        if (linkcosts.find(neighbor_node_ID) == linkcosts.end())
        {
            toErase.push_back(node_ID);
            result = true;
        }
    }
    for (auto ID: toErase)
        dvtable.erase(ID);

    if (result)
    {
        printDVTable();
        // update the routing table accordingly
        updateRoutingTableDV();
    }
    return result;
}

void RoutingProtocolImpl::updateRoutingTableDV()
{
    for (auto &line : dvtable)
    {
        unsigned short dest_ID = line.first;
        auto cost_hop = line.second;
        unsigned short hop = cost_hop.second;
        auto rit = routingtable.find(dest_ID);
        if (rit != routingtable.end())
            rit->second = hop;
        else
            routingtable.insert(pair<unsigned short, unsigned short>(dest_ID, hop));
    }
    vector<unsigned short> toErase = {};
    for (auto &line : routingtable)
    {
        unsigned short dest_ID = line.first;
        if (dvtable.find(dest_ID) == dvtable.end())
            toErase.push_back(dest_ID);
    }
    for (auto dest_ID : toErase)
        routingtable.erase(dest_ID);
}

void RoutingProtocolImpl::updateTable(unsigned short s_ID, char* packet, unsigned short size ){
    unordered_map<unsigned short, unsigned short>::iterator it = routingtable.find(s_ID);
    if (it != routingtable.end()) {
            unsigned short hop = it->second;
            unordered_map<unsigned short, LinkTable>::iterator link_it = linkmap.find(hop);
            if (link_it != linkmap.end()) {//routing table used for send on correct path
                LinkTable lnk = link_it->second;
                sys->send(lnk.port_ID, packet, size);
            }else{
            free(packet);
        }   
    } else {
        free(packet);
    }
}
//have links dropped or not responsive?
bool RoutingProtocolImpl::checkTopology(){
    bool ischange = false;
    unordered_map<unsigned short, LinkTable>::iterator it = linkmap.begin();
    set<unsigned short> changed_s_ID;
    while (it != linkmap.end()) {
            LinkTable lnk= it->second;
            if (sys->time() > lnk.expire_timeout) {//issue
                // Delete the entry in linkcosts table as well
                linkcosts.erase(it->first);
                ischange = true;
                changed_s_ID.insert(it->first);
                linkmap.erase(it++);
            } else {
                ++it;
            }   
    }
    if(ischange){
            if (this->protocol == P_LS) {
            	   ls->modifyLinkState(changed_s_ID);//what needs to be deleted?
                   ls->shortestPath(routingtable);//recalibrate shortest path
            } else {
                if (updateDVTable())
                    sendDVPacket();
            }
    }   
    return ischange;
}

void RoutingProtocolImpl::printDVTable()
{
    // The following block of code prints the dvtable
    cout << "---------------------------\n";
    cout << "| DV Table of node " << this->router_id << endl;
    for (auto line: dvtable)
    {
        cout << "| destination node " << line.first;
        auto cost_hop = line.second;
        cout << ", cost " << cost_hop.first << ", next hop " << cost_hop.second << endl;
    }
    cout << "---------------------------\n";
}

// add more of your own code

