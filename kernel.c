#include "kernel.h"

int appPids[5];
AppContext appContexts[5];
int appStates[5];

int executing[5];
int waitingIn1[5];
int waitingIn2[5];

int accessed1[5];
int accessed2[5];

int interFd;

AppContext* appContextShm;
int syscallFd;

int paused = 0;

// Func defs

void CreateInterController();
void CreateApps();

void SetState(int ap, int state);

void PrintAppStates();

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
        PrintAppStates();
    }
    else
    {
        LoadContext();
        printf("\n  Unpaused\n");
    }

    paused = !paused;
}

void InterSigHandler(int signal)
{
    if (paused)
        return;

    printf("\n-----\n");
    PrintAppStates();

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
    int info;
    while (read(syscallFd, &info, sizeof(int)) > 0)
    {
        SaveContext();

        if (info == FINISH)
            appStates[Pop(executing)] = FINISHED;
        else
        {
            int input = info;
            read(syscallFd, &info, sizeof(int));
            int access = info;

            appStates[executing[0]] = input + access * 10;

            if (input == D1)
                PushEnd(waitingIn1, Pop(executing));
            else if (input == D2)
                PushEnd(waitingIn2, Pop(executing));
        }
        LoadContext();
    }
}

//
// Main
//

int main()
{
    signal(SIGINT, CtrlcSigHandler);
    signal(SIGUSR1, InterSigHandler);
    signal(SIGUSR2, AppSigHandler);

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

    while(1)
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
    
    char arg1[11] = "0000000000";

    char* args[3] = {
        "./intercontroller",
        arg1,
        NULL
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

    char arg1[11] = "0000000000";
    char arg2[11] = "0000000000";

    char* args[4] = {
        "./ax",
        arg1,
        arg2,
        NULL
    };
    itoa(shm, args[1], 10);
    itoa(fd[1], args[2], 10);

    for (int i = 0; i < 5; i++)
    {
        executing[i] = i;
        appStates[i] = i == 0 ? EXECUTING : READY;

        int pid = fork();
        if (pid == 0)
        {
            close(fd[0]);
            execv("ax", args);
            exit(0);
        }

        appPids[i] = pid;
        if (i != 0)
            kill(pid, SIGSTOP);
    }

    close(fd[1]);
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

void PrintAppStates()
{
    for (int i = 0; i < 5; i++)
    {
        printf("AP%d:\n", i + 1);
        printf("  PC: %d\n", appContexts[i].PC);
        printf("  State: %s\n", AppStateToStr(appStates[i]));
        printf("  Accessed D1: %d times\n", accessed1[i]);
        printf("  Accessed D2: %d times\n", accessed2[i]);
    }
}

void SaveContext()
{
    if (executing[0] == -1)
        return;

    appContexts[executing[0]] = *appContextShm;

    kill(appPids[executing[0]], SIGSTOP);
}

void LoadContext()
{
    if (executing[0] == -1)
        return;
    
    *appContextShm = appContexts[executing[0]];

    kill(appPids[executing[0]], SIGCONT);
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
    for (int i = 0; i < 5; i++)
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

    for (int i = 4; i > 0; i--)
        arr[i] = arr[i - 1];

    arr[0] = val;
}