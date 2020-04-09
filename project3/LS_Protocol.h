#ifndef LS_PROTOCOL_H
#define LS_PROTOCOL_H

#include "global.h"
#include <arpa/inet.h>
#include <unordered_map>
#include <vector>

struct LS_Record {
  unsigned int expire_timeout;
  unsigned short hop_id;
  unsigned short linkcost;
};



class LS_Protocol{

	public:
	~LS_Protocol();	
	void setRouterID(unsigned short router_id);
	void updateLS(char* packet, unsigned int timeout, unsigned int time, unsigned short size);
	void updatePONG(unsigned short s_ID, unsigned short linkcost, unsigned int time);
	void shortestPath(unordered_map<unsigned short, unsigned short> &routingtable);
	void createLSPacket(char* packet, unsigned short packet_size);
	void increment();
	bool checkSeqNum(char* packet);
	LS_Record* checkLinkState(unsigned short s_ID);
	void changeTopology(unsigned short n_id);
	//id2seq map, recordtable and linkstate
	unordered_map<unsigned short, unsigned int> id2seq;
	unordered_map<unsigned short, vector<LS_Record*>*> recordtable;
	vector<LS_Record*> linkstate;

	private:
		
	unsigned short router_id;
	unsigned int seqnum;


};
#endif
