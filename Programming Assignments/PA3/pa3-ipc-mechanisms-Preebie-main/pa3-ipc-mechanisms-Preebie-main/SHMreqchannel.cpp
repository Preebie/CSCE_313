#include "common.h"
#include "SHMreqchannel.h"
using namespace std;

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR/DESTRUCTOR FOR CLASS   R e q u e s t C h a n n e l  */
/*--------------------------------------------------------------------------*/

SHMRequestChannel::SHMRequestChannel(const string _name, const Side _side, int _len) : RequestChannel (_name, _side){
	s1 = "/SHM_" + my_name + "1";
	s2 = "/SHM_" + my_name + "2";
	len = _len;

	sharedMessageQ1 = new SHMQ (s1, len);
	sharedMessageQ2 = new SHMQ (s2, len);

	if(my_side == CLIENT_SIDE) {
		swap(sharedMessageQ1, sharedMessageQ2);
	}
}

SHMRequestChannel::~SHMRequestChannel(){ 
	delete sharedMessageQ1;
	delete sharedMessageQ2;
}


int SHMRequestChannel::cread(void* msgbuf, int bufcapacity){
	return sharedMessageQ1->shm_recv(msgbuf, bufcapacity); 
}

int SHMRequestChannel::cwrite(void* msgbuf, int len){
	return sharedMessageQ2->shm_send(msgbuf, len);
}



