#include "LS_Protocol.h"


//initialize the router to start sending LS Packets
void LS_Protocol::setRouterID(unsigned short router_id)
{
	this->router_id = router_id;
	this->seqnum = 0;
	this->linkstate = {};
	this->id2seq = {};
};

//modify link state according to deleted/changes ID's
void LS_Protocol::modifyLinkState(set<unsigned short>& changed_s_ID){
	for (set<unsigned short>::iterator it = changed_s_ID.begin(); it != changed_s_ID.end(); ++it){
		vector<LS_Record*>::iterator rec_it = linkstate.begin();
		while(rec_it != linkstate.end()){
			LS_Record* rec = *rec_it;
			if(rec->hop_id	== *it){
				changeTopology(rec->hop_id);
				rec_it = linkstate.erase(rec_it);
				free(rec);
				break;
			} else {
				++rec_it;
			}
		}
	}
}

//initialize
LS_Protocol::LS_Protocol(unsigned short router_id)
{
	router_id = router_id;
	seqnum = 0;
};

//destructor - delete linkstate and recordtable as they were dynamically allocated
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

//have some links been deleted and not updated in the recordtable?
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

//check if any links are unresponsive?
bool LS_Protocol::checkLinkState(unsigned int time){
  	bool ischanged = false;
  	vector<LS_Record*>::iterator it = linkstate.begin();
  	while (it != linkstate.end()){
    		if ((*it)->expire_timeout < time) {
      			ischanged = true;
      			LS_Record* rec = *it;
      			changeTopology(rec->hop_id);
      			it = linkstate.erase(it);
      			free(rec);
    		} else {
      			++it;
    		}
  	}	
  	return ischanged;
}

//create LS Packet
void LS_Protocol::createLSPacket(char* packet, unsigned short packet_size){
	*(char*)packet = LS;
  	*(unsigned short*)(packet + 2) = (unsigned short)htons(packet_size);
  	*(unsigned short*)(packet + 4) = (unsigned short)htons(this->router_id);
  	*(unsigned int*)(packet + 8) = (unsigned int)htonl(this->seqnum);
  	/* get hop ID and linkcost from linkstate */
  	int count = 0;
  	for(vector<LS_Record*>::iterator it = linkstate.begin(); it != linkstate.end(); it++){
    		int offset = 12 + (count * 4);
    		*(unsigned short*)((char*)packet + offset) = (unsigned short)htons((*it)->hop_id);
    		*(unsigned short*)((char*)packet + offset + 2) = (unsigned short)htons((*it)->linkcost);
    		++count;
  	}
}

//return the reference to the same router
LS_Record* LS_Protocol::returnLinkState(unsigned short s_ID){
    for (auto &ls_rec : linkstate)
    {
    	if (ls_rec->hop_id == s_ID)
    		return ls_rec;
    }
    return nullptr;
}


//calculate shortest path and feed it into the routing table
void LS_Protocol::shortestPath(unordered_map<unsigned short, unsigned short>& routingtable){
	//store hop and cost
	unordered_map<unsigned short, pair<unsigned short, unsigned short> > hopcost;
	//clear routing table
	routingtable.clear();
	for(vector<LS_Record*>::iterator it = linkstate.begin(); it != linkstate.end(); it++){
		hopcost[(*it)->hop_id] = make_pair((*it)->linkcost,(*it)->hop_id);
	}
	//get minimum cost
	while(!hopcost.empty()){
    		unsigned short min_cost = INFINITY_COST; //infty defined in global.h
		unsigned short temp_id;
		unordered_map<unsigned short, pair<unsigned short, unsigned short> >::iterator hopit;
		for(hopit = hopcost.begin(); hopit != hopcost.end(); hopit++){
			if(hopit->second.first < min_cost){
				min_cost = hopit->second.first;
				temp_id = hopit->second.second;
			}	
		}
		//routingtable addition
		hopcost.erase(temp_id);
		routingtable[temp_id] = temp_id;
		unordered_map<unsigned short, vector<LS_Record*>*>::iterator recit = recordtable.find(temp_id);
		if(recit!=recordtable.end()){
			vector<LS_Record*>* rec_vec = recit->second;
			for(vector<LS_Record*>::iterator vecit = rec_vec->begin(); vecit != rec_vec->end(); vecit++){
				LS_Record* rec = *vecit;
				unsigned short path_cost = min_cost +  rec->linkcost;
				unordered_map<unsigned short, pair<unsigned short, unsigned short> >::iterator hopit_in;
				hopit_in = hopcost.find(rec->hop_id);
				if(hopit_in != hopcost.end()){
					//update path_cost
					unsigned short tempcost = hopit_in->second.first;
					if(path_cost < tempcost){
						hopcost[rec->hop_id] = make_pair(path_cost, temp_id);
					}
					
				} else {
					//add calculate path as it is seen first
					unordered_map<unsigned short, unsigned short>::iterator rit = routingtable.find(rec->hop_id);
					if(this->router_id != rec->hop_id && rit == routingtable.end()){
						hopcost[rec->hop_id] = make_pair(path_cost,temp_id);
					}
				}		
			}		
		}
	}
}


bool LS_Protocol::updatePONG(unsigned short s_ID, unsigned int timeout, unsigned short linkcost, unsigned int time){
	bool ischanged = false;
	LS_Record* rec = returnLinkState(s_ID);
	if(rec != nullptr){
		rec->expire_timeout = time + timeout;
		if(linkcost!=rec->linkcost){
			ischanged=true;
			rec->linkcost = linkcost; 
		}
	}
	else{ //New record add it top linkstate
		ischanged = true;
    		rec = static_cast<LS_Record*>(malloc(sizeof(LS_Record)));
		rec->hop_id = s_ID;
    		rec->linkcost = linkcost;
    		rec->expire_timeout = time + timeout;
		linkstate.push_back(rec);
	}
	return ischanged;
}

//ls alarm triggers update of the table upon receiving packet
void LS_Protocol::updateLS(char* packet, unsigned int timeout,  unsigned int time, unsigned short size){
	unsigned short s_ID = (unsigned short)ntohs(*(unsigned short*)(packet + 4));
  	unsigned int count = (size - 12) >> 2;
  	vector<LS_Record*>* ls_rec_vec = new vector<LS_Record*>;
  	for (unsigned int i = 0; i < count; ++i) {
    		unsigned int offset = 12 + (i * 4);
    		unsigned short hop_id = (unsigned short)ntohs(*(unsigned short*)(packet + offset));
    		if (hop_id== this->router_id) {
			//same router ignore
      			continue;
    		}
    		unsigned short linkcost = (unsigned short)ntohs(*(unsigned short*)(packet + offset + 2));
    		LS_Record* ls_rec = static_cast<LS_Record*>(malloc(sizeof(LS_Record)));
    		ls_rec->hop_id = hop_id;
    		ls_rec->linkcost = linkcost;
    		ls_rec_vec->push_back(ls_rec);
		//added all new linkcosts
  	}
  	LS_Record* my_hop_rec = returnLinkState(s_ID);
	if (my_hop_rec != nullptr) {
		//new timeout set
    		my_hop_rec->expire_timeout = time + timeout;
  	}
  	unordered_map<unsigned short, vector<LS_Record*>*>::iterator it = recordtable.find(s_ID);
	if(it == recordtable.end()){
		recordtable[s_ID] = ls_rec_vec;
	}else{
		//remove old entries related to s_ID
		vector<LS_Record*>* tempvec = it->second;
		vector<LS_Record*>::iterator tempit = tempvec->begin();
		while(tempit != tempvec->end()){
			LS_Record* rec = *tempit;
			tempit = tempvec->erase(tempit);
			free(rec);
		}
		delete(tempvec);
	}
	recordtable[s_ID] = ls_rec_vec;	
}

// seqnum needs to incremented
void LS_Protocol::increment(){
	this->seqnum++;
}

//check if we have seen an updated packet or a similar packet as before
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

