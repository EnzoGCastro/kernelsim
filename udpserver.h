#ifndef UDP_SERVER
#define UDP_SERVER

#include "udp.h"

int SetupUdpServer(int portno);

int SendMessage(char* message, int messageLen);
int ReceiveMessage(char* buff, int buffLen);

#endif
