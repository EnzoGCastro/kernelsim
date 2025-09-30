#include <stdio.h>
#include <stdlib.h>

#include "kernel.h"

#define MAX 100

AppContext* context;
int syscallFd; // pipe para dados do syscall

void _syscall(int Dx, int Op)
{
    write(syscallFd, Dx, sizeof(int));
    write(syscallFd, Op, sizeof(int));
    kill(SIGUSR1, getppid());
    context->PC++;
}

int main(int argc, char* argv[])
{
    context = shmat(atoi(argv[0]), 0, 0);
    syscallFd = dup(atoi(argv[1]));

    int Dx;
    int Op;

    while (context->PC < MAX)
    {
        sleep(0.5);
        int d = rand() % 100;
        if (d < 15)
        { // generate a random syscall
            if (d % 2)
                Dx = D1;
            else
                Dx= D2;

            if (d % 3 == 1)
                Op = R;
            else if (d % 3 = 1)
                Op = W;
            else
                Op = X;
            
            _syscall(Dx, Op);
        }
        sleep(0.5);
    }

    return 0;
}
