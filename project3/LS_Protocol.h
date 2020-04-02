#ifndef LS_PROTOCOL_H
#define LS_PROTOCOL_H

#include "global.h"
#include <arpa/inet.h>

class LS_Protocol{

	public:
	
	void setRouterID(unsigned short router_id);

	private:
	
	unsigned short router_id;
	unsigned short seqnum;


};
#endif
