#include "RoutingProtocolImpl.h"
#include <string.h>

RoutingProtocolImpl::RoutingProtocolImpl(Node *n) : RoutingProtocol(n) {
  sys = n;
  // add your own code
}

RoutingProtocolImpl::~RoutingProtocolImpl() {
  // add your own code (if needed)
}

void RoutingProtocolImpl::init(unsigned short num_ports, unsigned short router_id, eProtocolType protocol_type) {
  // set private variables
  this->num_ports = num_ports;
  this->router_id = router_id;
  this->protocol = protocol_type; //enum defined in global.h - imported in RoutingProtocol.h

}

void RoutingProtocolImpl::handle_alarm(void *data) {
  // add your own code
  char* alarmtype = reinterpret_cast<char*>(data);
  if (strcmp(alarmtype,"ping") == 0) pingTime();
  else if(strcmp(alarmtype, "pong") == 0) pongTime();
  else if(strcmp(alarmtype, "distancevector") == 0) dvTime();
  else if(strcmp(alarmtype, "linkstatepacket") == 0) lsTime();
  else if(strcmp(alarmtype, "update") == 0) updateTime();
  else {
	fprintf(stderr, "Not able to recognize %s alarm", alarmtype);
	exit(EXIT_FAILURE);
  }
}

void RoutingProtocolImpl::recv(unsigned short port, void *packet, unsigned short size) {
  // add your own code
  // need to free recv after processing
  // need to check what kind of packet has been received - there are 5 different kinds of packets!
}

// add more of your own code
