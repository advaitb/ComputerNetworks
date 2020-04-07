#ifndef LS_PROTOCOL_H
#define LS_PROTOCOL_H

#include "global.h"
#include <arpa/inet.h>
#include <unordered_map>
#include <vector>

class LS_Protocol{

	public:
	
	void setRouterID(unsigned short router_id);
	void updateLS(char* packet, unsigned int time, unsigned short size);
	void updatePONG(unsigned short s_ID, unsigned short linkcost, unsigned int time);
	void shortestPath(unordered_map<unsigned short, unsigned short> &routingtable);
	void createLSPacket(char* packet, unsigned short packet_size);
	unordered_map<unsigned short, unsigned int> id2seq;
		

	private:
		
	unsigned short router_id;
	unsigned int seqnum;


};
#endif
