#include "kernel.h"

int interFd; // pipe para sinais de clock e input

void CtrlcSigHandler(int signal){
    return;
}

int main(int argc, char* argv[])
{
    signal(SIGINT, CtrlcSigHandler);
    interFd = atoi(argv[1]);
    int parentId = getppid();

    int w = 0;

    while(1)
    {
        usleep(TIME_FRAME_USEC);

        w = CLOCK;
        write(interFd, &w, sizeof(int));
        
        int randomNum = rand() % (1000); 

        if (randomNum < SEND_D1_CHANCE_1000)
        {
            w = D1;
            write(interFd, &w, sizeof(int));
        }

        randomNum = rand() % (1000); 

        if (randomNum < SEND_D2_CHANCE_1000)
        {
            w = D2;
            write(interFd, &w, sizeof(int));
        }

        kill(parentId, SIGUSR1);
    }
}
