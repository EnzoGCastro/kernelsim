/*
 * udpserver.c - A simple UDP echo server
 * usage: udpserver <port>
 */

#include "udpserver.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int sockfd;
socklen_t clientlen;
struct sockaddr_in clientaddr;
struct hostent *hostp;
char *hostaddrp;

/*
 * error - wrapper for perror
 */
void Error(char *msg)
{
  perror(msg);
  exit(1);
}

int SetupUdpServer(int portno)
{
    struct sockaddr_in serveraddr; /* server's addr */
    int optval; /* flag value for setsockopt */

    /*
    * socket: create the parent socket
    */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
        Error("ERROR opening socket");
    
    /* setsockopt: Handy debugging trick that lets
    * us rerun the server immediately after we kill it;
    * otherwise we have to wait about 20 secs.
    * Eliminates "ERROR on binding: Address already in use" error.
    */
    optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));

    /*
    * build the server's Internet address
    */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short)portno);

    /*
    * bind: associate the parent socket with a port
    */
    if (bind(sockfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0)
        Error("ERROR on binding");

    clientlen = sizeof(clientaddr);

    return 0;
}

int SendMessage(char* message, int messageLen)
{
    int n = sendto(sockfd, message, messageLen, 0, (const struct sockaddr *) &clientaddr, clientlen);
    if (n < 0)
        Error("ERROR in sendto");
    return n;
}

int ReceiveMessage(char* buff, int buffLen)
{
    int n = recvfrom(sockfd, buff, buffLen, 0, (struct sockaddr* restrict) &clientaddr, &clientlen);
    if (n < 0)
        Error("ERROR in recvfrom");
    return n;
}
