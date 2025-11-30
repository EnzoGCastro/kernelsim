/* 
 * udpclient.c - A simple UDP client
 * usage: udpclient <host> <port>
 */

#include "udpclient.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

int sockfd;
socklen_t serverlen;
struct sockaddr_in serveraddr;

/* 
 * error - wrapper for perror
 */
void Error(char *msg)
{
    perror(msg);
    exit(0);
}

int SetupUdpClient(char* hostname, int portno)
{
    struct hostent *server;

    /* socket: create the socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
        Error("ERROR opening socket");

    /* gethostbyname: get the server's DNS entry */
    server = gethostbyname(hostname);
    if (server == NULL)
    {
        fprintf(stderr,"ERROR, no such host as %s\n", hostname);
        exit(0);
    }

    /* build the server's Internet address */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
    (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons((unsigned short)portno);

    serverlen = sizeof(serveraddr);

    return 0;
}

int SendMessage(char* message, int messageLen)
{
    int n = sendto(sockfd, message, messageLen, 0, (const struct sockaddr*) &serveraddr, serverlen);
    if (n < 0)
        Error("ERROR in sendto");
    return n;
}

int ReceiveMessage(char* buff, int buffLen)
{
    return recvfrom(sockfd, buff, buffLen, MSG_DONTWAIT, (struct sockaddr* restrict) &serveraddr, &serverlen);
}
