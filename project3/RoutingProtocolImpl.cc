#include "RoutingProtocolImpl.h"
#include <string.h>
#include "DV_Protocol.h"
#include "LS_Protocol.h"

#define PING_PACK_SIZE 9
#define PONG_PACK_SIZE 5

// init node
RoutingProtocolImpl::RoutingProtocolImpl(Node *n) : RoutingProtocol(n) {
  sys = n;
}

// deconstructor
RoutingProtocolImpl::~RoutingProtocolImpl() {
  // add your own code (if needed)
}

// set the alarm
void RoutingProtocolImpl::setAlarmType( RoutingProtocol *r, unsigned int duration, void *d){
	sys->set_alarm(r,duration,d);
}

void RoutingProtocolImpl::init(unsigned short num_ports, unsigned short router_id, eProtocolType protocol_type) {
  // set private variables
  
  if( protocol_type != P_LS || protocol_type != P_DV) {
	fprintf(stderr, "Incorrect protocol initialization\n");
	exit(EXIT_FAILURE); 
  }

  this->num_ports = num_ports;
  this->router_id = router_id;
  this->protocol = protocol_type; //enum defined in global.h - imported in RoutingProtocol.h
  
  
  setAlarmType(this, 0, (void*)this->ping); //initialize ping
  setAlarmType(this, checkalarm, (void*)this->update); 
  
  switch(this->protocol){
	case P_LS:
		ls->setRouterID(this->router_id);
		setAlarmType(this, lsalarm, (void*)this->linkstate);
		break;
	case P_DV:
		dv->setRouterID(this->router_id);
		setAlarmType(this, dvalarm, (void*)this->distancevector); 
		break;
  }
}

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

void RoutingProtocolImpl::recv(unsigned short port, void *packet, unsigned short size) {
  // add your own code
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

void RoutingProtocolImpl::pingTime(){
	//TODO
	int i;
	for(i=0;i<this->num_ports;i++){
		sendPingPacket(i);
	}
	setAlarmType(this, pingalarm, (void*)this->ping); 
}

void RoutingProtocolImpl::lsTime(){
	//TODO
}

void RoutingProtocolImpl::dvTime(){
	//TODO
}

void RoutingProtocolImpl::updateTime(){
	//TODO
}

void RoutingProtocolImpl::sendPingPacket(int port){
	char* ping_packet = (char*)malloc(PING_PACK_SIZE);
	*ping_packet = (char)PING;
	*(unsigned short*)(ping_packet + 1) = (unsigned short)htons(PING_PACK_SIZE);
	*(unsigned short*)(ping_packet + 3) = (unsigned short)htons(this->router_id);
	*(unsigned int*)(ping_packet + 5) = (unsigned int)htonl(sys->time());
	sys->send(port, ping_packet, PING_PACK_SIZE);
}

void RoutingProtocolImpl::recvDataPacket(char* packet, unsigned short size){
	//TODO
}
void RoutingProtocolImpl::recvPingPacket(unsigned short port, char* packet, unsigned short size){
	//need to send back a PONG to the  router that sent the PING
	unsigned short packet_size = (unsigned short)ntohs(*(unsigned short*)(packet + 1));
	if(packet_size != PING_PACK_SIZE){
		free(packet);
		return;
	}
	unsigned short recv_id = (unsigned short)ntohs(*(unsigned short*)(packet + 3));       
	char* pong_packet = (char*)malloc(PONG_PACK_SIZE);
	*pong_packet = (char)PONG;
  	*(unsigned short*)(pong_packet + 1) = (unsigned short)htons(this->router_id);
  	*(unsigned short*)(pong_packet + 3) = (unsigned short)htons(recv_id);
	free(packet);
        sys->send(port, pong_packet, PONG_PACK_SIZE);
}
void RoutingProtocolImpl::recvPongPacket(unsigned short port, char* packet){
	//pong packets
	if(this->router_id != (unsigned short)ntohs(*(unsigned short*)(packet+3))){
		free(packet);
		return;
	}
	unsigned int sendtime = (unsigned int)ntohl(*(unsigned int*)(packet + 5));
  	unsigned short linkcost = (short)(sys->time() - sendtime);
  	unsigned short src_id = (unsigned short)ntohs(*(unsigned short*)(packet + 3));
  	unsigned int time_to_expire = sys->time() + pongto;
  	free(packet);		
	//TODO - NEED TO UPDATE TABLE

}
void RoutingProtocolImpl::recvLSPacket(unsigned short port, char* packet, unsigned short size){
	//TODO
}
void RoutingProtocolImpl::recvDVPacket(char* packet, unsigned short size){
		//TODO
}
// add more of your own code
