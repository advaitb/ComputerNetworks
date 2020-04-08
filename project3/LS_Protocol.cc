#include "LS_Protocol.h"

void LS_Protocol::setRouterID(unsigned short router_id)
{
	this->router_id = router_id;
	this->seqnum = 0;
};

LS_Protocol::~LS_Protocol(){
	vector<LS_Record*>::iterator lnkst_it = linkstate.begin();
	while(lnkst_it != linkstate.end()){
		LS_Record* rec = *lnkst_it;
		lnkst_it = linkstate.erase(lnkst_it);
		free(rec);
	}
	unordered_map<unsigned short, vector<LS_Record*>*>::iterator it = recordtable.begin();
	while (it != recordtable.end()) {
    		vector<LS_Record*>* vec = it->second;
    		recordtable.erase(it++);
    		vector<LS_Record*>::iterator vec_it = vec->begin();
    		while (vec_it != vec->end()) {
      			LS_Record* rec = *vec_it;
      			vec_it = vec->erase(vec_it);
      			free(rec);
    		}
    		delete vec;
  	}	
}

void LS_Protocol::createLSPacket(char* packet, unsigned short packet_size){
	*(char*)packet = LS;
  	*(unsigned short*)(packet + 2) = (unsigned short)htons(packet_size);
  	*(unsigned short*)(packet + 4) = (unsigned short)htons(this->router_id);
  	*(unsigned int*)(packet + 8) = (unsigned int)htonl(this->seqnum);
  	/* get hop ID and linkcost from linkstate */
  	int count = 0;
  	for(vector<LS_Record*>::iterator it = linkstate.begin(); it != linkstate.end(); it++){
    		int offset = 12 + (count << 2);
    		*(unsigned short*)((char*)packet + offset) = (unsigned short)htons((*it)->hop_id);
    		*(unsigned short*)((char*)packet + offset + 2) = (unsigned short)htons((*it)->linkcost);
    		++count;
  	}
}

void LS_Protocol::changeTopology(unsigned short n_ID){
	unordered_map<unsigned short, vector<LS_Record*>*>::iterator it = recordtable.find(n_ID);
	if(it == recordtable.end()) return;
	vector<LS_Record*>* rec_vec = it->second;
	vector<LS_Record*>::iterator rec_it = rec_vec->begin();
	while(rec_it != rec_vec->end()){
		LS_Record* rec = *rec_it;
		rec_it = rec_vec->erase(rec_it);
		free(rec);
	}
	recordtable.erase(it);
	delete rec_vec;
}


LS_Record* LS_Protocol::checkLinkState(unsigned short s_ID){
	for(vector<LS_Record*>::iterator iter = linkstate.begin(); iter != linkstate.end(); ++iter) {
    		LS_Record* rec  = *iter;
    		if (rec->hop_id == s_ID) {
      			return rec;
    		}
	}
	return nullptr;
}

void LS_Protocol::updateLS(char* packet, unsigned int timeout,  unsigned int time, unsigned short size){
	unsigned short s_ID = (unsigned short)ntohs(*(unsigned short*)(packet + 4));
  	unsigned int count = (size - 12) >> 2;
  	vector<LS_Record*>* ls_rec_vec = new vector<LS_Record*>;
  	for (unsigned int i = 0; i < count; ++i) {
    		unsigned int offset = 12 + (i << 2);
    		unsigned short hop_id = (unsigned short)ntohs(*(unsigned short*)(packet + offset));
    		if (hop_id== this->router_id) {
      			continue;
    		}
    		unsigned short linkcost = (unsigned short)ntohs(*(unsigned short*)(packet + offset + 2));
    		LS_Record* ls_rec = static_cast<LS_Record*>(malloc(sizeof(LS_Record)));
    		ls_rec->hop_id = hop_id;
    		ls_rec->linkcost = linkcost;
    		ls_rec_vec->push_back(ls_rec);
  	}
  	LS_Record* my_hop_rec = checkLinkState(s_ID);
  	if (my_hop_rec != nullptr) {
    		my_hop_rec->expire_timeout = time + timeout;
  	}
  	unordered_map<unsigned short, vector<LS_Record*>*>::iterator it = recordtable.find(s_ID);
	if(it == recordtable.end()){
		recordtable[s_ID] = ls_rec_vec;
	}else{
		vector<LS_Record*>* tempvec = it->second;
		vector<LS_Record*>::iterator tempit = tempvec->begin();
		while(tempit != tempvec->end()){
			LS_Record* rec = *tempit;
			tempit = tempvec->erase(tempit);
			free(rec);
		}
		delete(tempvec);
	}	
}


void LS_Protocol::increment(){
	this->seqnum++;
}

bool LS_Protocol::checkSeqNum(char* ls_packet){
	unsigned short s_ID = (unsigned short)ntohs(*(unsigned short*)(ls_packet + 4));
	unsigned int seqNum = (unsigned int)ntohl(*(unsigned int*)(ls_packet + 8));
	if (s_ID == this->router_id) {
    		return false;
	}
	
	if(id2seq.find(s_ID) == id2seq.end() || id2seq[s_ID] < seqNum){
    		id2seq[s_ID]=seqNum;
    		return true;
  	}
  	return false;
}
