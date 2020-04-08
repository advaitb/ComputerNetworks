#ifndef ROUTINGPROTOCOLIMPL_H
#define ROUTINGPROTOCOLIMPL_H

#include "RoutingProtocol.h"
#include "Node.h"
#include "LS_Protocol.h"
#include "DV_Protocol.h"
#include <unordered_map>
#include <set>
#define pingalarm 10000  //10s
#define lsalarm 30000    //30s
#define dvalarm 30000    //30s
#define checkalarm 1000  //1s
#define pongto 15000     //15s
#define lsto 45000       //45s
#define dvto 45000       //45s


struct LinkTable{
  unsigned int expire_timeout;
  unsigned short port_ID;
};


class RoutingProtocolImpl : public RoutingProtocol {
  public:
    RoutingProtocolImpl(Node *n);
    ~RoutingProtocolImpl();

    void init(unsigned short num_ports, unsigned short router_id, eProtocolType protocol_type);
    // As discussed in the assignment document, your RoutingProtocolImpl is
    // first initialized with the total number of ports on the router,
    // the router's ID, and the protocol type (P_DV or P_LS) that
    // should be used. See global.h for definitions of constants P_DV
    // and P_LS.

    void handle_alarm(void *data);
    // As discussed in the assignment document, when an alarm scheduled by your
    // RoutingProtoclImpl fires, your RoutingProtocolImpl's
    // handle_alarm() function will be called, with the original piece
    // of "data" memory supplied to set_alarm() provided. After you
    // handle an alarm, the memory pointed to by "data" is under your
    // ownership and you should free it if appropriate.

    void recv(unsigned short port, void *packet, unsigned short size);
    // When a packet is received, your recv() function will be called
    // with the port number on which the packet arrives from, the
    // pointer to the packet memory, and the size of the packet in
    // bytes. When you receive a packet, the packet memory is under
    // your ownership and you should free it if appropriate. When a
    // DATA packet is created at a router by the simulator, your
    // recv() function will be called for such DATA packet, but with a
    // special port number of SPECIAL_PORT (see global.h) to indicate
    // that the packet is generated locally and not received from 
    // a neighbor router.
    
    // Alarms fired!
    void pingTime();//time for pinging neighbors
    void updateTime();//update time
    void lsTime();//lstable update
    void dvTime();//dvtable update 
    
    void setAlarmType(RoutingProtocol *r, unsigned int duration, void *d);//sys->set_alarm
    //recv packets - main recv will call each of these functions
    void recvDataPacket(char* packet, unsigned short size);
    void recvPingPacket(unsigned short port, char* packet, unsigned short size);
    void recvPongPacket(unsigned short port, char* packet);
    void recvLSPacket(unsigned short port, char* packet, unsigned short size);
    void recvDVPacket(char* packet, unsigned short size);
    void updateDVTable();
    void verify(char* packet, unsigned short size);
    bool checkTopology();    
    //send packets
    //PONG sent in recvPingPacket
    void sendPingPacket(int port);
    void sendLSPacket();
    void sendDVPacket(unsigned short port_ID, unsigned short d_ID);
    //Update Table
    void updateTable(unsigned short s_ID, char* packet, unsigned short size);
    
    //protocol pointers
    DV_Protocol* dv;
    LS_Protocol* ls;
    
    //Link information
    unordered_map<unsigned short,LinkTable> linkmap;
    //RoutingTable
    unordered_map<unsigned short, unsigned short> routingtable;

    // Link costs
    // key: neighbor ID. value: cost
    unordered_map<unsigned short, unsigned short> linkcosts;    

    // DV Table
    // key: dest_ID. value: (cost, next_hop)
    unordered_map<unsigned short, pair<unsigned short, unsigned short>> dvtable;
  
  private:
    Node *sys; // To store Node object; used to access GSR9999 interfaces 
    unsigned short num_ports;
    unsigned short router_id;
    eProtocolType protocol;
    const char* ping = "ping";
    const char* distancevector = "distancevector";
    const char* linkstate = "linkstate";
    const char* update = "update";
    //const char indicating packet type
    //const char DATA = 0x00;
    //const char PING = 0x01;
    //const char PONG = 0x02;
    //const char LS = 0x03;
    //const char DV = 0x04;
};
#endif

