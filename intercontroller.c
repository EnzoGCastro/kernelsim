#include "kernel.h"

int interFd; // pipe para sinais de clock e input

int main(int argc, char* argv[])
{
    interFd = atoi(argv[1]);
    int parentId = getppid();

    int w = 0;

    while(1)
    {
        usleep(500000);

        w = CLOCK;
        write(interFd, &w, sizeof(int));
        
        int randomNum = rand() % (1000); 

        if (randomNum < 100)
        {
            w = D1;
            write(interFd, &w, sizeof(int));
        }

        randomNum = rand() % (1000); 

        if (randomNum < 5)
        {
            w = D2;
            write(interFd, &w, sizeof(int));
        }

        kill(parentId, SIGUSR1);
    }
}
