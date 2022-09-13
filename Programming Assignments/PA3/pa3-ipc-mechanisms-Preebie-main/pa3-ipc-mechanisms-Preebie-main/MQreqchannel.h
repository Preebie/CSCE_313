
#ifndef _MQreqchannel_H_
#define _MQreqchannel_H_

#include "common.h"
#include "RequestChannel.h"

class MQRequestChannel: public RequestChannel {
public:
	MQRequestChannel(const string _name, const Side _side);
	~MQRequestChannel();
	int cread (void* msgbuf, int bufcapacity);
	int cwrite (void *msgbuf , int msglen);
	int open_ipc(string _pipe_name, int mode);
};

#endif
