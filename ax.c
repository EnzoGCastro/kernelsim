#include <stdio.h>
#include <stdlib.h>

#define MAX 100

#define D1 1
#define D2 2

#define R 1
#define W 2
#define X 3

int ppid;
int* PC;
int syscallFd; // pipe para dados do syscall

void _syscall(int Dx, int Op)
{
    write(syscallFd, Dx, sizeof(int));
    write(syscallFd, Op, sizeof(int));
    signal(SIGUSR1, ppid);
    PC++;
}

int main(int argc, char* argv[])
{
    ppid = atoi(argv[0]);
    PC = shmat(atoi(argv[1]), 0, 0);
    syscallFd = dup(atoi(argv[2]));

    int Dx;
    int Op;

    while (PC < MAX)
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