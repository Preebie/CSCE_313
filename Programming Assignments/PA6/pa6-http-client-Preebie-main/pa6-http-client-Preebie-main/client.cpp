#include <algorithm>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

using namespace std;
int socket_creation(const string hostname, const string port_no = "80")
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

    int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
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
    return sockfd;
}

void string_processor(string url, string filestring)
{
    string hostname = "";
    string path = "";
    if (url.substr(0, 7) == "http://")
    {
        url = url.substr(7);
    }
    else if (url.substr(0, 8) == "https://")
    {
        url = url.substr(8);
    }
    if (url.find("/") != string::npos)
    {
        hostname = url.substr(0, url.find("/"));
        path = url.substr(url.find("/"));
    }
    else
    {
        hostname = url;
    }
    int sockfd = socket_creation(hostname);
    char buf[2048];
    for (int i = 0; i < 2048; i++)
    {
        buf[i] = 0;
    }

    string request = "GET " + path + " HTTP/1.1\r\nHost: " + hostname + "\r\n\r\n";
    const char *requestbuf = request.c_str();
    send(sockfd, requestbuf, request.size(), 0);

    string receive = "";

    while (recv(sockfd, buf, 2047, 0) != 0)
    {
        for (int i = 0; i < 2048; i++)
        {
            if (buf[i] == '\0')
            {
                continue;
            }
            receive += (static_cast<char>(buf[i]));
        }
        memset(buf, 0, 2048);
    }
    string header = receive.substr(0, receive.find("\r\n\r\n"));
    string text = receive.substr(receive.find("\r\n\r\n") + 4);
    char first_digit = header[9];
    ofstream filer(filestring, ios_base::trunc);
    if (first_digit == '2')
    {
        if (header.find("transfer-encoding: chunked") != string::npos)
        {
            int current = 0;
            for (int i = 0; i < text.size() - 5; i++)
            {
                if (text[i] == '\r' && text[i + 1] == '\n')
                {
                    current += 1;
                    i += 2;
                }
                if (current % 2 == 1 && current != 0)
                {
                    filer << text[i];
                }
            }
        }
        else
        {
            filer << text;
        }
        filer.close();
    }
    else if (first_digit == '3')
    {
        filer.close();
        string new_location = header.substr(header.find("location: ") + 9);
        string_processor(new_location, filestring);
        return;
    }
    else if (first_digit == '4' || first_digit == '5')
    {
        filer.close();
        exit(1);
    }
}

int main(int argc, char **argv)
{
    string url = string(argv[1]);
    string filename = "experiment";

    if (argc != 2 and argc != 1)
    {
        filename = string(argv[2]);
    }

    string_processor(url, filename);
}
