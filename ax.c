#include "kernel.h"

AppData* data;
int syscallFd; // pipe para dados do syscall
int myPid;

void _syscall(char* info, int infoLen)
{
    while (pthread_mutex_trylock(&(data->appMutex)))
    {}

    data->appSendingData = 1;

    write(syscallFd, &infoLen, sizeof(int));
    write(syscallFd, info, sizeof(char) * infoLen);
    raise(SIGSTOP);
}

void CtrlcSigHandler(int signal)
{
    return;
}

int main(int argc, char* argv[])
{
    signal(SIGINT, CtrlcSigHandler);
    data = shmat(atoi(argv[1]), 0, 0);
    syscallFd = atoi(argv[2]);

    data->context.PC = 0;

    srand(time(NULL) + getpid());

    myPid = getpid();
    int Dx;
    int Op;
    while (data->context.PC < PC_MAX)
    {
        usleep(TIME_FRAME_USEC);
        data->context.PC++;
        
        int d = rand() % 100;
        if (d < REQUEST_INPUT_CHANCE_100)
        {
            // generate a random syscall

            if (d % 2)
                Dx = D1;
            else
                Dx= D2;

            d = d % 5;
            switch(d)
            {
            case(0):
                Op = WT;
                break;
            case(1):
                Op = RD;
                break;
            case(2):
                Op = AD;
                break;
            case(3):
                Op = RM;
                break;
            case(4):
                Op = LS;
                break;
            }
                
            char info[2] = {
                Dx,
                Op
            };
            _syscall(info, 2);
        }
    }
    
    char finishInfo[1] = {
        FINISH
    };
    _syscall(finishInfo, 1);

    return 0;
}
