#include "kernel.h"

#include "udpclient.h"

int appPids[5];
AppContext appContexts[5];
int appStates[5];

int executing[5];
int waitingIn1[5];
int waitingIn2[5];
int isBlocked[5];

int accessed1[5];
int accessed2[5];

char updMessageIds[5];
int udpWaitingMessage[5];
double udpMessageResendTimer[5];
char udpLastMessage[5][MAX_UDP_MESSAGE_SIZE];
int udpLastMessageSize[5];

char udpRecMessage[MAX_UDP_MESSAGE_SIZE];

int interFd;

AppData* appData;
int syscallFd;
int syscallReturnFds[5];

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
            if (waitingIn1[0] == -1 || isBlocked[waitingIn1[0]])
                continue;

            SaveContext();
            SetState(executing[0], READY);
            int waiter = Pop(waitingIn1);
            accessed1[waiter] += 1;
            PushStart(executing, waiter);
            SetState(executing[0], EXECUTING);
            LoadContext();
        }
        else if (info == D2)
        {
            if (waitingIn2[0] == -1 || isBlocked[waitingIn2[0]])
                continue;

            SaveContext();
            SetState(executing[0], READY);
            int waiter = Pop(waitingIn2);
            accessed2[waiter] += 1;
            PushStart(executing, waiter);
            SetState(executing[0], EXECUTING);
            LoadContext();
        }
    }

    PrintAppStates();
    printf("\n");
    printf("Executing [%d, %d, %d, %d, %d]\n", executing[0], executing[1], executing[2], executing[3], executing[4]);
    printf("Waiting 1 [%d, %d, %d, %d, %d]\n", waitingIn1[0], waitingIn1[1], waitingIn1[2], waitingIn1[3], waitingIn1[4]);
    printf("Waiting 2 [%d, %d, %d, %d, %d]\n", waitingIn2[0], waitingIn2[1], waitingIn2[2], waitingIn2[3], waitingIn2[4]);
    printf("\n");

    pthread_mutex_unlock(&(appData->appMutex));
}

void AppSigHandler(int signal)
{
    if (appData->appSendingData == 0)
        return;

    int infoLen;
    read(syscallFd, &infoLen, sizeof(int));

    char info[MAX_SYSCALL_INFO_SIZE];
    char* infoP = info;
    read(syscallFd, info, sizeof(char) * infoLen);
    
    SaveContext();

    int instruction = infoP[0];
    infoP++;

    switch (instruction)
    {
    case FINISH:
        syscallFINISH(infoP);
        break;
    case WT:
        syscallWT(infoP);
        break;
    case RD:
        syscallRD(infoP);
        break;
    case AD:
        syscallAD(infoP);
        break;
    case RM:
        syscallRM(infoP);
        break;
    case LS:
        syscallLS(infoP);
        break;

    default:
        printf("ERROR invalid syscall\n");
        break;
    }

    SetState(executing[0], EXECUTING);

    pthread_mutex_unlock(&(appData->appMutex));
    appData->appSendingData = 0;

    LoadContext();
}

//
// UDP
//

void SendUdpMessage(int appId)
{
    udpWaitingMessage[appId] = 1;
    udpMessageResendTimer[appId] = CLOCK_TIME_MS;
    SendMessage(udpLastMessage[appId], udpLastMessageSize[appId]);
}

void ReceiveUdpMessage()
{
    // formato (id, owner, instruction, ...

    char* message = udpRecMessage;

    char id = *message;
    message++;

    int ax = *message;
    message++;

    if (updMessageIds[ax] != id)
        return;

    char instruction = *message;
    message++;
    
    char error;

    switch (instruction)
    {
    case SFSS_WRITE:
        // continuacao formato ... error)

        error = *message;
        message++;
        write(syscallReturnFds[ax], &error, sizeof(char));

        break;
    
    case SFSS_READ:
        // continuacao formato ... error, [se tiver erro para aqui] payload)

        error = *message;
        message++;
        write(syscallReturnFds[ax], &error, sizeof(char));

        if (!error)
        {
            write(syscallReturnFds[ax], message, sizeof(char) * PAYLOAD_BLOCK_SIZE);
            message += PAYLOAD_BLOCK_SIZE;
        }

        break;

    case SFSS_ADDDIR:
        // continuacao formato ... error)

        error = *message;
        message++;
        write(syscallReturnFds[ax], &error, sizeof(char));

        break;

    case SFSS_REMOVE:
        // continuacao formato ... error)

        error = *message;
        message++;
        write(syscallReturnFds[ax], &error, sizeof(char));

        break;

    case SFSS_LIST:
        // continuacao formato ... error, [se tiver erro para aqui] total dirs, dir starts <n>, dirs <n>)

        error = *message;
        message++;
        write(syscallReturnFds[ax], &error, sizeof(char));

        if (!error)
        {
            int total;
            memcpy(&total, message, sizeof(int));
            write(syscallReturnFds[ax], &total, sizeof(int));
            message += sizeof(int);

            int totalSent = total < MAX_LIST_DIRS ? total : MAX_LIST_DIRS;
            write(syscallReturnFds[ax], message, sizeof(int) * totalSent);
            message += sizeof(int) * totalSent;

            for (int i = 0; i < totalSent; i++)
            {
                char dir[MAX_DIR_LEN];
                strcpy(dir, message);
                write(syscallReturnFds[ax], dir, strlen(dir) + 1);
                message += strlen(dir) + 1;
            }
        }

        break;
    }

    updMessageIds[ax] = (updMessageIds[ax] + 1) % 256;
    udpWaitingMessage[ax] = 0;
    isBlocked[ax] = 0;
}

void ProcessUdpMessages()
{
    while (ReceiveMessage(udpRecMessage, MAX_UDP_MESSAGE_SIZE) > 0)
        ReceiveUdpMessage();

    for (int i = 0; i < 5; i++)
        if (udpWaitingMessage[i] && udpMessageResendTimer[i] < CLOCK_TIME_MS - UDP_REQ_RESEND_DURATION_MSEC)
            SendUdpMessage(i); // Resend
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
        isBlocked[i] = 0;
        appStates[i] = -1;
        accessed1[i] = 0;
        accessed2[i] = 0;
        updMessageIds[i] = 0;
        syscallReturnFds[i] = -1;
    }

    SetupUdpClient("127.0.0.1", UDP_PORT);

    CreateInterController();
    CreateApps();

    while(1)
        ProcessUdpMessages();

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
    char arg3[11] = "0000000000";

    char* args[5] = {
        "./ax",
        arg1,
        arg2,
        arg3,
        NULL
    };
    itoa(shm, args[1], 10);
    itoa(fd[1], args[2], 10);

    for (int i = 0; i < 5; i++)
    {
        executing[i] = i;
        appStates[i] = i == 0 ? EXECUTING : READY;

        int returnFd[2];
        pipe(returnFd);
        syscallReturnFds[i] = returnFd[1];
        itoa(returnFd[0], args[3], 10);

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

        close(returnFd[0]);
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

//
// Process syscalls
//

void syscallFINISH(char* info)
{
    SetState(Pop(executing), FINISHED);
}

void syscallWT(char* info)
{
    int ax = Pop(executing);
    PushEnd(waitingIn1, ax);
    SetState(ax, WAITING_WT);
    isBlocked[ax] = 1;
    
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
    for (int i = 0; i < payloadSize * PAYLOAD_BLOCK_SIZE; i++)
    {
        payload[i] = *info;
        info++;
    }

    unsigned char offset = *info;

    // udp package (id, owner, write, path, payloadSize, payload, offset)

    char* message = udpLastMessage[ax];
    int messageSize = 0;

    message[messageSize] = updMessageIds[ax];
    messageSize++;

    message[messageSize] = ax;
    messageSize++;

    message[messageSize] = SFSS_WRITE;
    messageSize++;

    strcpy(message + messageSize, path);
    messageSize += strlen(path) + 1;

    message[messageSize] = payloadSize;
    messageSize++;

    for (int i = 0; i < payloadSize * PAYLOAD_BLOCK_SIZE; i++)
    {
        message[messageSize] = payload[i];
        messageSize++;
    }

    message[messageSize] = offset;
    messageSize++;

    udpLastMessageSize[ax] = messageSize;
    
    SendUdpMessage(ax);

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
    int ax = Pop(executing);
    PushEnd(waitingIn1, ax);
    SetState(ax, WAITING_RD);
    isBlocked[ax] = 1;
    
    char path[MAX_PATH_LEN];
    for (int i = 0; i < MAX_PATH_LEN; i++)
    {
        path[i] = *info;
        info++;
        if (path[i] == '\0')
            break;
    }

    unsigned char offset = *info;

    char* message = udpLastMessage[ax];
    int messageSize = 0;

    message[messageSize] = updMessageIds[ax];
    messageSize++;

    message[messageSize] = ax;
    messageSize++;

    message[messageSize] = SFSS_READ;
    messageSize++;

    strcpy(message + messageSize, path);
    messageSize += strlen(path) + 1;
    
    message[messageSize] = offset;
    messageSize++;

    udpLastMessageSize[ax] = messageSize;
    
    SendUdpMessage(ax);

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
    int ax = Pop(executing);
    PushEnd(waitingIn2, ax);
    SetState(ax, WAITING_AD);
    isBlocked[ax] = 1;

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

    char* message = udpLastMessage[ax];
    int messageSize = 0;

    message[messageSize] = updMessageIds[ax];
    messageSize++;

    message[messageSize] = ax;
    messageSize++;

    message[messageSize] = SFSS_ADDDIR;
    messageSize++;

    strcpy(message + messageSize, path);
    messageSize += strlen(path) + 1;
    
    strcpy(message + messageSize, dir);
    messageSize += strlen(dir) + 1;

    udpLastMessageSize[ax] = messageSize;
    
    SendUdpMessage(ax);

    /*
    printf("kernel - add ---\n");
    printf("path: %s\n", path);
    printf("dir: %s\n", dir);
    printf("---\n");
    */
}

void syscallRM(char* info)
{
    int ax = Pop(executing);
    PushEnd(waitingIn2, ax);
    SetState(ax, WAITING_RM);
    isBlocked[ax] = 1;

    char path[MAX_PATH_LEN];
    for (int i = 0; i < MAX_PATH_LEN; i++)
    {
        path[i] = *info;
        info++;
        if (path[i] == '\0')
            break;
    }

    char* message = udpLastMessage[ax];
    int messageSize = 0;

    message[messageSize] = updMessageIds[ax];
    messageSize++;

    message[messageSize] = ax;
    messageSize++;

    message[messageSize] = SFSS_REMOVE;
    messageSize++;

    strcpy(message + messageSize, path);
    messageSize += strlen(path) + 1;

    udpLastMessageSize[ax] = messageSize;
    
    SendUdpMessage(ax);

    /*
    printf("kernel - remove ---\n");
    printf("path: %s\n", path);
    printf("---\n");
    */
}

void syscallLS(char* info)
{
    int ax = Pop(executing);
    PushEnd(waitingIn2, ax);
    SetState(ax, WAITING_LS);
    isBlocked[ax] = 1;

    char path[MAX_PATH_LEN];
    for (int i = 0; i < MAX_PATH_LEN; i++)
    {
        path[i] = *info;
        info++;
        if (path[i] == '\0')
            break;
    }

    char* message = udpLastMessage[ax];
    int messageSize = 0;

    message[messageSize] = updMessageIds[ax];
    messageSize++;

    message[messageSize] = ax;
    messageSize++;

    message[messageSize] = SFSS_LIST;
    messageSize++;

    strcpy(message + messageSize, path);
    messageSize += strlen(path) + 1;

    udpLastMessageSize[ax] = messageSize;
    
    SendUdpMessage(ax);

    /*
    printf("kernel - list ---\n");
    printf("path: %s\n", path);
    printf("fd: %d\n", fd);
    printf("---\n");
    */
}