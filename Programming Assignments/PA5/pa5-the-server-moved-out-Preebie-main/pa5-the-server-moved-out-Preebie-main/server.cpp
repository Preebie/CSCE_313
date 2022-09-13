#include <cassert>
#include <cstring>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <sys/types.h>
#include <sys/stat.h>

#include <thread>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <vector>
#include <math.h>
#include "TCPreqchannel.h"

#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <thread>
using namespace std;

int buffercapacity = MAX_MESSAGE;
char *buffer = NULL; 

int nchannels = 0;
pthread_mutex_t newchannel_lock;
void handle_process_loop(TCPRequestChannel *_channel);
char ival;
vector<string> all_data[NUM_PERSONS];

void populate_file_data(int person)
{
	
	string filename = "BIMDC/" + to_string(person) + ".csv";
	char line[100];
	ifstream ifs(filename.c_str());
	if (ifs.fail())
	{
		EXITONERROR("Data file: " + filename + " does not exist in the BIMDC/ directory");
	}
	int count = 0;
	while (!ifs.eof())
	{
		line[0] = 0;
		ifs.getline(line, 100);
		if (ifs.eof())
			break;
		double seconds = stod(split(string(line), ',')[0]);
		if (line[0])
			all_data[person - 1].push_back(string(line));
	}
}

double get_data_from_memory(int person, double seconds, int ecgno)
{
	int index = (int)round(seconds / 0.004);
	string line = all_data[person - 1][index];
	vector<string> parts = split(line, ',');
	double sec = stod(parts[0]);
	double ecg1 = stod(parts[1]);
	double ecg2 = stod(parts[2]);
	if (ecgno == 1)
		return ecg1;
	else
		return ecg2;
}

void process_file_request(TCPRequestChannel *rc, char *request)
{

	filemsg f = *(filemsg *)request;
	string filename = request + sizeof(filemsg);
	filename = "BIMDC/" + filename; 
	

	if (f.offset == 0 && f.length == 0)
	{ 
		__int64_t fs = get_file_size(filename);
		rc->cwrite((char *)&fs, sizeof(__int64_t));
		return;
	}


	char *response = request;

	
	if (f.length > buffercapacity)
	{
		cerr << "Client is requesting a chunk bigger than server's capacity" << endl;
		cerr << "Returning nothing (i.e., 0 bytes) in response" << endl;
		rc->cwrite(response, 0);
	}
	FILE *fp = fopen(filename.c_str(), "rb");
	if (!fp)
	{
		cerr << "Server received request for file: " << filename << " which cannot be opened" << endl;
		rc->cwrite(buffer, 0);
		return;
	}
	fseek(fp, f.offset, SEEK_SET);
	int nbytes = fread(response, 1, f.length, fp);

	assert(nbytes == f.length);
	rc->cwrite(response, nbytes);
	fclose(fp);
}

void process_data_request(TCPRequestChannel *rc, char *request)
{
	datamsg *d = (datamsg *)request;
	double data = get_data_from_memory(d->person, d->seconds, d->ecgno);
	rc->cwrite((char *)&data, sizeof(double));
}

void process_unknown_request(TCPRequestChannel *rc)
{
	char a = 0;
	rc->cwrite(&a, sizeof(a));
}

int process_request(TCPRequestChannel *rc, char *_request)
{
	MESSAGE_TYPE m = *(MESSAGE_TYPE *)_request;
	if (m == DATA_MSG)
	{
		usleep(rand() % 5000);
		process_data_request(rc, _request);
	}
	else if (m == FILE_MSG)
	{
		process_file_request(rc, _request);
	}
	else
	{
		process_unknown_request(rc);
	}
}

void handle_process_loop(TCPRequestChannel *channel)
{

	char *buffer = new char[buffercapacity];
	if (!buffer)
	{
		EXITONERROR("Cannot allocate memory for server buffer");
	}
	while (true)
	{
		int nbytes = channel->cread(buffer, buffercapacity);
		if (nbytes < 0)
		{
			cerr << "Client-side terminated abnormally" << endl;
			break;
		}
		else if (nbytes == 0)
		{
			
			cerr << "Could not read anything" << endl;
			break;
		}
		MESSAGE_TYPE m = *(MESSAGE_TYPE *)buffer;
		if (m == QUIT_MSG)
		{
			break;
			
		}
		process_request(channel, buffer);
	}
	delete[] buffer;
	delete channel;
}

int main(int argc, char *argv[])
{
	buffercapacity = MAX_MESSAGE;
	int opt;
	string portnum;
	while ((opt = getopt(argc, argv, "m:r:")) != -1)
	{
		switch (opt)
		{
		case 'm':
			buffercapacity = atoi(optarg);
			cout << "buffer capacity is " << buffercapacity << " bytes" << endl;
			break;
		case 'r':
			portnum = optarg;
			break;
		}
	}
	srand(time_t(NULL));
	for (int i = 0; i < NUM_PERSONS; i++)
	{
		populate_file_data(i + 1);
	}

	TCPRequestChannel *control_channel = new TCPRequestChannel("", portnum);
	struct sockaddr_storage their_addr; 
	socklen_t sin_size;
	
	while (1)
	{ 
		sin_size = sizeof their_addr;
		int client_socket = accept(control_channel->getfd(), (struct sockaddr *)&their_addr, &sin_size);
		if (client_socket == -1)
		{
			perror("accept");
			continue;
		}
		TCPRequestChannel* new_chan = new TCPRequestChannel(client_socket);
		thread t(handle_process_loop, new_chan);
		t.detach();
	}
	cout << "Server terminated" << endl;
	
}
