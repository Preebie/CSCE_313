
#ifndef _FIFOreqchannel_H_
#define _FIFOreqchannel_H_

#include "common.h"
#include "RequestChannel.h"

class FIFORequestChannel: public RequestChannel {
public:
	FIFORequestChannel(const string _name, const Side _side);
	~FIFORequestChannel();
	int cread (void* msgbuf, int bufcapacity);
	int cwrite (void *msgbuf , int msglen);
	int open_ipc(string _pipe_name, int mode);
};

#endif
