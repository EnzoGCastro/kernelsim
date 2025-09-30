#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/shm.h>

#include "kernel.h"

int appPids[5];
AppContext appContexts[5];

int executing[5];
int waitingIn1[5];
int waitingIn2[5];

int* appContextShm;
int syscallFd;

void ctrlcSigHandler(int signal)
{

}

void appSigHandler(int signal)
{

}

int main()
{
    signal(SIGINT, ctrlcSigHandler);
    signal(SIGUSR1, appSigHandler);

    int shm = shmget(IPC_PRIVATE, sizeof(AppContext), IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
    appContextShm = (AppContext*)shmat(shm, 0, 0);

    int fd[2];
    pipe(fd);
    syscallFd = fd[0];

    char* appArgs[2] = {
        "0000000000",
        "0000000000"
    };
    appArgs[0] = itoa(shm);
    appArgs[1] = itoa(fd[1]);

    for (int i = 0; i < 5; i++)
    {
        executing[i] = i;
        waitingIn1[i] = -1;
        waitingIn2[i] = -1;

        int pid = fork();
        if (pid == 0)
        {
            execv("ax", appArgs);
            return 0;
        }

        appPids[i] = pid;
        kill(SIGSTOP, pid);
    }

    return 0;
}
