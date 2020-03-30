#include "RoutingProtocolImpl.h"
#include <string.h>

RoutingProtocolImpl::RoutingProtocolImpl(Node *n) : RoutingProtocol(n) {
  sys = n;
}

RoutingProtocolImpl::~RoutingProtocolImpl() {
  // add your own code (if needed)
}

void RoutingProtocolImpl::init(unsigned short num_ports, unsigned short router_id, eProtocolType protocol_type) {
  // set private variables
  this->num_ports = num_ports;
  this->router_id = router_id;
  this->protocol = protocol_type; //enum defined in global.h - imported in RoutingProtocol.h
  

  if(this->protocol == P_LS){
  //TODO
  
  }
  else if(this->protocol ==  P_DV ){
  //TODO
  
  }
  else{
	fprintf(stderr, "Not able to recognize protocol, please choose between LS and DV\n");
	exit(EXIT_FAILURE);
}

void RoutingProtocolImpl::handle_alarm(void *data) {
  char* alarmtype = reinterpret_cast<char*>(data);
  if (strcmp(alarmtype,this->ping) == 0) pingTime();
  else if(strcmp(alarmtype, this->pong) == 0) pongTime();
  else if(strcmp(alarmtype, this->distancevector) == 0) dvTime();
  else if(strcmp(alarmtype, this->linkstate) == 0) lsTime();
  else if(strcmp(alarmtype, this->update) == 0) updateTime();
  else {
	fprintf(stderr, "Not able to recognize %s alarmi\n", alarmtype);
	exit(EXIT_FAILURE);
  }
}

void RoutingProtocolImpl::recv(unsigned short port, void *packet, unsigned short size) {
  // add your own code
  // need to free recv after processing
  // need to check what kind of packet has been received - there are 5 different kinds of packets!
}

void RoutingProtocolImpl::pingTime(){
	//TODO
}

void RoutingProtocolImpl::pongTime(){
	//TODO
}

void RoutingProtocolImpl::lsTime(){
	//TODO
}

void RoutiingProtocolImpl::dvTime(){
	//TODO
}

void RoutingProtocolImpl::updateTime(){
	//TODO
}


// add more of your own code
