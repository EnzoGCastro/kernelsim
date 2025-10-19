#include "kernel.h"

AppContext* context;
int syscallFd; // pipe para dados do syscall
int myPid;

void _syscall(int Dx, int Op)
{
    int info[3] = {
        myPid,
        Dx,
        Op
    };
    write(syscallFd, &info, sizeof(int) * 3);
    kill(getppid(), SIGUSR2);
}

void CtrlcSigHandler(int signal){
    return;
}

int main(int argc, char* argv[])
{
    signal(SIGINT, CtrlcSigHandler);
    context = shmat(atoi(argv[1]), 0, 0);
    syscallFd = atoi(argv[2]);

    context->PC = 0;

    srand(time(NULL) + getpid());

    myPid = getpid();
    int Dx;
    int Op;
    while (context->PC < PC_MAX)
    {
        usleep(TIME_FRAME_USEC);
        context->PC++;
        
        int d = rand() % 100;
        if (d < REQUEST_INPUT_CHANCE_100)
        { // generate a random syscall
            if (d % 2)
                Dx = D1;
            else
                Dx= D2;

            if (d % 3 == 1)
                Op = R;
            else if (d % 3 == 2)
                Op = W;
            else
                Op = X;
            
            _syscall(Dx, Op);
        }
    }

    int info[2] = {
        myPid,
        FINISH
    };
    write(syscallFd, &info, sizeof(int) * 2);
    kill(getppid(), SIGUSR2);

    return 0;
}
