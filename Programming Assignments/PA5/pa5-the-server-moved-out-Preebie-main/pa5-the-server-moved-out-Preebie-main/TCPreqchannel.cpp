#include "common.h"
#include "TCPreqchannel.h"
#include <sys/socket.h>
#include <netdb.h>

using namespace std;

TCPRequestChannel::TCPRequestChannel(const string hostname, const string port_no)
{

	if (hostname != "")
	{
		struct addrinfo hints, *res;

		memset(&hints, 0, sizeof hints);
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		int status;
		if ((status = getaddrinfo(hostname.c_str(), port_no.c_str(), &hints, &res)) != 0)
		{
			cerr << "getaddrinfo: " << gai_strerror(status) << endl;
			exit(1);
		}

		sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (sockfd < 0)
		{
			perror("Cannot create socket");
			exit(1);
		}

		if (connect(sockfd, res->ai_addr, res->ai_addrlen) < 0)
		{
			perror("Cannot Connect");
			exit(1);
		}
	}

	else
	{
		struct addrinfo hints, *serv;
		struct sockaddr_storage their_addr; //connector's address information
		socklen_t sin_size;
		char s[INET6_ADDRSTRLEN];
		int rv;

		memset(&hints, 0, sizeof hints);
		hints.ai_family = AF_UNSPEC;	 //any address family IPv4, IPv6
		hints.ai_socktype = SOCK_STREAM; //stream oriented socket
		hints.ai_flags = AI_PASSIVE;	 //use all available IP in this device

		if ((rv = getaddrinfo(NULL, port_no.c_str(), &hints, &serv)) != 0)
		{
			cerr << "getaddrinfo: " << gai_strerror(rv) << endl;
			exit(1);
		}
		if ((sockfd = socket(serv->ai_family, serv->ai_socktype, serv->ai_protocol)) == -1)
		{
			perror("server: socket");
			exit(1);
		}
		if (bind(sockfd, serv->ai_addr, serv->ai_addrlen) == -1)
		{
			close(sockfd);
			perror("server: bind");
			exit(1);
		}
		freeaddrinfo(serv); // all done with this structure

		if (listen(sockfd, 20) == -1)
		{
			perror("listen");
			exit(1);
		}
	}
}

TCPRequestChannel::TCPRequestChannel(int sockfd)
{
	this->sockfd = sockfd;
}
TCPRequestChannel::~TCPRequestChannel()
{
	close(this->sockfd);
}
int TCPRequestChannel::cread(void *msgbuf, int buflen)
{
	return recv(this->sockfd, msgbuf, buflen, 0);
}
int TCPRequestChannel::cwrite(void *msgbuf, int msglen)
{
	return send(this->sockfd, msgbuf, msglen, 0);
}
int TCPRequestChannel::getfd()
{
	return this->sockfd;
}
