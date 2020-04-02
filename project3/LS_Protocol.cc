#include "LS_Protocol.h"

void LS_Protocol::setRouterID(unsigned short router_id)
{
	this->router_id = router_id;
	this->seqnum = 0;
};
