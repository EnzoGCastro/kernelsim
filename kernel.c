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

int GetPidIndex(int pid);

int Pop(int* arr);
int PopIndex(int* arr, int index);
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
                continue;

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
                continue;

            SaveContext();
            SetState(executing[0], READY);
            int waiter = Pop(waitingIn2);
            accessed2[waiter] += 1;
            PushStart(executing, waiter);
            LoadContext();
        }
    }

    printf("\n-----\n");
    PrintAppStates();
}

void AppSigHandler(int signal)
{
    int pid;
    while (read(syscallFd, &pid, sizeof(int)) > 0)
    {
        int info;
        read(syscallFd, &info, sizeof(int));

        SaveContext();

        if (info == FINISH)
        {
            int i = GetPidIndex(pid);
            int* arr;
            if (appStates[i] == READY || appStates[i] == EXECUTING)
                arr = executing;
            else if (appStates[i] % 10 == D1)
                arr = waitingIn1;
            else if (appStates[i] % 10 == D2)
                arr = waitingIn2;
            appStates[PopIndex(arr, i)] = FINISHED;
        }
        else
        {
            int access;
            read(syscallFd, &access, sizeof(int));

            appStates[executing[0]] = info + (access * 10);

            if (info == D1)
                PushEnd(waitingIn1, PopIndex(executing, GetPidIndex(pid)));
            else if (info == D2)
                PushEnd(waitingIn2, PopIndex(executing, GetPidIndex(pid)));
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

    while(1){}

    return 0;
}

//
// Starters
//

void CreateInterController()
{
    int fd[2];
    pipe(fd);
    int flags = fcntl(fd[0], F_GETFL, 0);
    fcntl(fd[0], F_SETFL, flags | O_NONBLOCK);
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
    int flags = fcntl(fd[0], F_GETFL, 0);
    fcntl(fd[0], F_SETFL, flags | O_NONBLOCK);
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

int GetPidIndex(int pid)
{
    for (int i = 0; i < 5; i++)
        if (appPids[i] == pid)
            return i;
    return -1;
}

int PopIndex(int* arr, int index)
{
    for (int i = 0; i < 5; i++)
    {
        if(arr[i] == index)
        {
            arr[i] = -1;
            for (int j = i; j < 4; j++)
            {
                arr[j] = arr[j+1];
                arr[j+1] = -1;
            }
            break;
        }
    }

    return index;
}
