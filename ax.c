#include "kernel.h"

AppData* data;
int syscallFd; // pipe para dados do syscall
int myPid;

void _syscall(int Dx, int Op)
{
    while (pthread_mutex_trylock(&(data->appMutex)))
    {}

    data->appSendingData = 1;
    int info[2] = {
        Dx,
        Op
    };
    write(syscallFd, &info, sizeof(int) * 2);
    raise(SIGSTOP);
}

void CtrlcSigHandler(int signal){
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
    
    _syscall(FINISH, 0);

    return 0;
}
