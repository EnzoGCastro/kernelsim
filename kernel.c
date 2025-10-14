#include "kernel.h"

int appPids[5];
AppContext appContexts[5];
int appStates[5];

int executing[5];
int waitingIn1[5];
int waitingIn2[5];

int accessed1[0];
int accessed2[0];

int interFd;

AppContext* appContextShm;
int syscallFd;

int paused = 0;

// Func defs

void CreateInterController();
void CreateApps();

void SetState(int ap, int state);

void SaveContext();
void LoadContext();

int Pop(int* arr);
void PushEnd(int* arr, int val);
void PushStart(int* arr, int val);

//
// Signals
//

void CtrlcSigHandler(int signal)
{
    if (paused == 0)
    {
        SaveContext();

        for (int i = 0; i < 5; i++)
        {
            printf("AP%d:\n", i + 1);
            printf("  PC: %d\n", appContexts[i].PC);
            printf("  State: %s\n", AppStateToStr(appStates[i]));
            printf("  Accessed D1: %d times\n", accessed1[i]);
            printf("  Accessed D2: %d times\n", accessed2[i]);
        }
    }
    else
    {
        LoadContext();
    }

    paused = !paused;
}

void InterSigHandler(int signal)
{
    if (paused)
        return;

    int info;
    while (read(interFd, &info, sizeof(int)) > 0)
    {
        if (info == CLOCK)
        {
            if (executing[1] == -1)
                continue;

            SaveContext();
            SetState(executing[0], READY);
            PushEnd(executing, Pop(executing));
            SetState(executing[0], EXECUTING);
            LoadContext();
        }
        else if (info == D1)
        {
            if (waitingIn1[0] == -1)
                return;

            SaveContext();
            SetState(executing[0], READY);
            int waiter = Pop(waitingIn1);
            accessed1[waiter] += 1;
            PushStart(executing, waiter);
            

            LoadContext();
        }
        else if (info == D2)
        {
            if (waitingIn2[0] == -1)
                return;

            SaveContext();
            SetState(executing[0], READY);

            int waiter = Pop(waitingIn2);
            accessed2[waiter] += 1;
            PushStart(executing, waiter);
            LoadContext();
        }
    }
}

void AppSigHandler(int signal)
{
    int info[2];
    while (read(syscallFd, &info, sizeof(int) * 2) > 0)
    {
        SaveContext();

        appStates[executing[0]] = info[0] + info[1] * 10;

        if (info[0] == D1)
            PushEnd(waitingIn1, Pop(executing));
        else if (info[0] == D2)
            PushEnd(waitingIn2, Pop(executing));

        LoadContext();
    }
}

void FinishedSigHandler(int signal)
{
    SaveContext();
    appStates[Pop(executing)] = FINISHED;
    LoadContext();
}

//
// Main
//

int main()
{
    signal(SIGINT, CtrlcSigHandler);
    signal(SIGUSR1, InterSigHandler);
    signal(SIGUSR2, AppSigHandler);
    signal(SIGCHLD, FinishedSigHandler);

    for (int i = 0; i < 5; i++)
    {
        executing[i] = -1;
        waitingIn1[i] = -1;
        waitingIn2[i] = -1;
        appStates[i] = -1;
        accessed1[i] = 0;
        accessed2[i] = 0;
    }

    CreateInterController();
    CreateApps();

    pause();

    return 0;
}

//
// Starters
//

void CreateInterController()
{
    int fd[2];
    pipe(fd);
    interFd = fd[0];

    char* args[2] = {
        "",
        "0000000000"
    };
    itoa(fd[1], args[1], 10);

    int pid = fork();
    if (pid == 0)
    {
        close(fd[0]);
        execv("intercontroller", args);
        exit(0);
    }

    close(fd[1]);
}

void CreateApps()
{
    int shm = shmget(IPC_PRIVATE, sizeof(AppContext), IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
    appContextShm = (AppContext*)shmat(shm, 0, 0);
    
    int fd[2];
    pipe(fd);
    syscallFd = fd[0];

    char* args[3] = {
        "",
        "0000000000",
        "0000000000"
    };
    itoa(shm, args[1], 10);
    itoa(fd[1], args[2], 10);

    for (int i = 0; i < 5; i++)
    {
        executing[i] = i;
        waitingIn1[i] = -1;
        waitingIn2[i] = -1;
        appStates[i] = READY;

        int pid = fork();
        if (pid == 0)
        {
            close(fd[0]);
            execv("ax", args);
            exit(0);
        }

        appPids[i] = pid;
        kill(SIGSTOP, pid);
    }

    close(fd[1]);

    appStates[0] = EXECUTING;
    kill(SIGCONT, appPids[0]);
}

//
// Aux 
//

void SetState(int app, int state)
{
    if (app == -1)
        return;

    appStates[app] = state;
}

void SaveContext()
{
    if (executing[0] == -1)
        return;

    appContexts[executing[0]] = *appContextShm;

    kill(SIGSTOP, appPids[executing[0]]);
}

void LoadContext()
{
    if (executing[0] == -1)
        return;
    
    *appContextShm = appContexts[executing[0]];

    kill(SIGCONT, appPids[executing[0]]);
}

int Pop(int* arr)
{

    int temp = arr[0];
    
    arr[0] = -1;
    for (int i = 0; i < 4; i++)
    {
        arr[i] = arr[i+1];
        arr[i+1] = -1;
    }
    
    return temp;
}

void PushEnd(int* arr, int val)
{

    for (int i = 5; i > -1; i--)
    {
        if (arr[i] == -1)
        {
            arr[i] = val;
            return;
        }
    }
}

void PushStart(int* arr, int val)
{
    if (val == -1)
        return;

    for (int i = 5; i >= 0; i--)
        arr[i] = arr[i-1];

    arr[0] = val;

    return;
}