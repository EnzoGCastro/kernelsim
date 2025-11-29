#ifndef UDP_CLIENT
#define UDP_CLIENT

#include "udp.h"

int SetupUdpClient(char* hostname, int portno);

int SendMessage(char* message, int messageLen);
int ReceiveMessage(char* buff, int buffLen);

#endif
