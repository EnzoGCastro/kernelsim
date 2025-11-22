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

AppData* appData;
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

void syscallFINISH(char* info);
void syscallWT(char* info);
void syscallRD(char* info);
void syscallAD(char* info);
void syscallRM(char* info);
void syscallLS(char* info);

//
// Signals
//

void CtrlcSigHandler(int signal)
{
    if (paused == 0)
    {
        paused = 1;
        SaveContext();
        PrintAppStates();
    }
    else
    {
        LoadContext();
        printf("\n  Unpaused\n");
        paused = 0;
    }

}

void InterSigHandler(int signal)
{
    if (paused)
        return;

    if (pthread_mutex_trylock(&(appData->appMutex)))
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
            printf("D1\n");
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
            printf("D2\n");
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

    pthread_mutex_unlock(&(appData->appMutex));

    PrintAppStates();
    printf("\n");
}

void AppSigHandler(int signal)
{
    if (appData->appSendingData == 0)
        return;

    int infoLen;
    read(syscallFd, &infoLen, sizeof(int));

    char info[MAX_SYSCALL_INFO_SIZE];
    read(syscallFd, info, sizeof(char) * infoLen);
    
    SaveContext();

    switch (info[0])
    {
    case FINISH:
        syscallFINISH(info);
        break;
    case WT:
        syscallWT(info);
        break;
    case RD:
        syscallRD(info);
        break;
    case AD:
        syscallAD(info);
        break;
    case RM:
        syscallRM(info);
        break;
    case LS:
        syscallLS(info);
        break;
    }

    pthread_mutex_unlock(&(appData->appMutex));
    appData->appSendingData = 0;

    LoadContext();
}

//
// Main
//

int main()
{
    signal(SIGINT, CtrlcSigHandler);
    signal(SIGUSR1, InterSigHandler);
    signal(SIGCHLD, AppSigHandler);

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

    while(1){
    }

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
    int shm = shmget(IPC_PRIVATE, sizeof(AppData), IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
    appData = (AppData*)shmat(shm, 0, 0);

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

    kill(appPids[executing[0]], SIGSTOP);
    appContexts[executing[0]] = appData->context;
}

void LoadContext()
{
    if (executing[0] == -1)
        return;

    appData->context = appContexts[executing[0]];
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

void syscallFINISH(char* info)
{
    appStates[Pop(executing)] = FINISHED;
}

void syscallWT(char* info)
{
    info++;

    char path[MAX_PATH_LEN];
    for (int i = 0; i < MAX_PATH_LEN; i++)
    {
        path[i] = *info;
        info++;
        if (path[i] == '\0')
            break;
    }

    unsigned char payloadSize = *info;
    info++;

    char payload[PAYLOAD_BLOCK_SIZE * PAYLOAD_MAX_BLOCKS];
    for (int i = 0; i < payloadSize; i++)
    {
        payload[i] = *info;
        info++;
    }

    unsigned char offset = *info;

    /*
    printf("kernel - write ---\n");
    printf("path: %s\n", path);
    printf("payloadSize: %d\n", payloadSize);
    printf("payload: ");
    for (int i = 0; i < payloadSize; i++)
        printf("%c", payload[i]);
    printf("\n");
    printf("offset: %d\n", offset);
    printf("---\n");
    */
}

void syscallRD(char* info)
{
    info++;

    char path[MAX_PATH_LEN];
    for (int i = 0; i < MAX_PATH_LEN; i++)
    {
        path[i] = *info;
        info++;
        if (path[i] == '\0')
            break;
    }

    char charFd[4];
    for (int i = 0; i < 4; i++)
    {
        charFd[i] = *info;
        info++;
    }
    int fd = *((int*)charFd);

    unsigned char offset = *info;

    /*
    printf("kernel - read ---\n");
    printf("path: %s\n", path);
    printf("fd: %d\n", fd);
    printf("offset: %d\n", offset);
    printf("---\n");
    */
}

void syscallAD(char* info)
{
    info++;

    char path[MAX_PATH_LEN];
    for (int i = 0; i < MAX_PATH_LEN; i++)
    {
        path[i] = *info;
        info++;
        if (path[i] == '\0')
            break;
    }

    char dir[MAX_DIR_LEN];
    for (int i = 0; i < MAX_PATH_LEN; i++)
    {
        dir[i] = *info;
        info++;
        if (dir[i] == '\0')
            break;
    }

    /*
    printf("kernel - add ---\n");
    printf("path: %s\n", path);
    printf("dir: %s\n", dir);
    printf("---\n");
    */
}

void syscallRM(char* info)
{
    info++;

    char path[MAX_PATH_LEN];
    for (int i = 0; i < MAX_PATH_LEN; i++)
    {
        path[i] = *info;
        info++;
        if (path[i] == '\0')
            break;
    }

    /*
    printf("kernel - remove ---\n");
    printf("path: %s\n", path);
    printf("---\n");
    */
}

void syscallLS(char* info)
{
    info++;

    char path[MAX_PATH_LEN];
    for (int i = 0; i < MAX_PATH_LEN; i++)
    {
        path[i] = *info;
        info++;
        if (path[i] == '\0')
            break;
    }

    char charFd[4];
    for (int i = 0; i < 4; i++)
    {
        charFd[i] = *info;
        info++;
    }
    int fd = *((int*)charFd);

    /*
    printf("kernel - list ---\n");
    printf("path: %s\n", path);
    printf("fd: %d\n", fd);
    printf("---\n");
    */
}