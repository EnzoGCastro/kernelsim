#include "kernel.h"

AppData* data;
int syscallFd; // pipe para escrever dados do syscall
int syscallReturnFd; // pipe para ler retorno do syscall
int myPid;

const int FILE_PATH_SIZE = 4;
const char* const FILE_PATHS[] = {
    "/f1.txt",
    "/f2.txt",
    "/dir/f3.txt",
    "/dir/dir/f4.txt"
};

const int AD_PATH_SIZE = 2;
const char* const AD_PATHS[] = {
    "/",
    "/dir"
};
const int AD_NAME_SIZE = 1;
const char* const AD_NAMES[] = {
    "/dir"
};

const int RM_PATH_SIZE = 6;
const char* const RM_PATHS[] = {
    "/f1.txt",
    "/f2.txt",
    "/dir",
    "/dir/f3.txt",
    "/dir/dir",
    "/dir/dir/f4.txt"
};

const int LS_PATH_SIZE = 3;
const char* const LS_PATHS[] = {
    "/",
    "/dir",
    "/dir/dir"
};

void _syscall(char* info, int infoLen);

void callWrite();
void callRead();
void callAdd();
void callRemove();
void callList();
void callFinish();

void CtrlcSigHandler(int signal)
{ return; }

int main(int argc, char* argv[])
{
    signal(SIGINT, CtrlcSigHandler);
    data = shmat(atoi(argv[1]), 0, 0);
    syscallFd = atoi(argv[2]);
    syscallReturnFd = atoi(argv[3]);

    data->context.PC = 0;

    srand(time(NULL) + getpid());

    myPid = getpid();
    while (data->context.PC < PC_MAX)
    {
        usleep(TIME_FRAME_USEC);
        data->context.PC++;
        
        int d = rand() % 100;
        if (d < REQUEST_INPUT_CHANCE_100)
        {
            // generate a random syscall

            d = d % 13;
            // 5x mais chance de chamar read ou write
            if (d < 5)
                callWrite();
            else if (d < 10)
                callRead();
            else if (d < 11)
                callAdd();
            else if (d < 12)
                callRemove();
            else
                callList();
        }
    }
    
    callFinish();

    return 0;
}

void _syscall(char* info, int infoLen)
{
    while (pthread_mutex_trylock(&(data->appMutex)))
    {}

    data->appSendingData = 1;

    write(syscallFd, &infoLen, sizeof(int));
    write(syscallFd, info, sizeof(char) * infoLen);
    raise(SIGSTOP);
}

void callWrite()
{
    char info[MAX_SYSCALL_INFO_SIZE];
    int size = 0;

    info[size] = WT;
    size++;

    const char* path = FILE_PATHS[rand() % FILE_PATH_SIZE];
    strcpy(info + size, path);
    size += strlen(path) + 1;

    unsigned  char payloadSize = (rand() % PAYLOAD_MAX_BLOCKS) + 1;
    info[size] = payloadSize;
    size++;

    char payload[PAYLOAD_BLOCK_SIZE * PAYLOAD_MAX_BLOCKS];
    int p = rand() % 10 + '0';
    for (int i = 0; i < payloadSize * PAYLOAD_BLOCK_SIZE; i++)
    {
        payload[i] = p;
        info[size] = payload[i];
        size++;
    }

    unsigned char offset = rand() % PAYLOAD_MAX_BLOCKS;
    info[size] = offset;
    size++;
    
    _syscall(info, size);

    char error;
    read(syscallReturnFd, &error, sizeof(char));

    if (error)
        printf("Write error at %s\n", path);
    else
        printf("Write success at %s\n", path);
}

void callRead()
{
    char info[MAX_SYSCALL_INFO_SIZE];
    int size = 0;

    info[size] = RD;
    size++;

    const char* path = FILE_PATHS[rand() % FILE_PATH_SIZE];
    strcpy(info + size, path);
    size += strlen(path) + 1;

    unsigned char offset = rand() % PAYLOAD_MAX_BLOCKS;
    info[size] = offset;
    size++;

    _syscall(info, size);

    char error;
    read(syscallReturnFd, &error, sizeof(char));

    if (error)
        printf("Read error at %s\n", path);
    else
    {
        char payload[PAYLOAD_BLOCK_SIZE];
        read(syscallReturnFd, payload, sizeof(char) * PAYLOAD_BLOCK_SIZE);
        printf("Read success at %s, value: ", path);
        for (int i = 0; i < PAYLOAD_BLOCK_SIZE; i++)
            printf("%c", payload[i]);
        printf("\n");
    }
}

void callAdd()
{
    char info[MAX_SYSCALL_INFO_SIZE];
    int size = 0;

    info[size] = AD;
    size++;

    const char* path = AD_PATHS[rand() % AD_PATH_SIZE];
    strcpy(info + size, path);
    size += strlen(path) + 1;

    const char* dir = AD_NAMES[rand() % AD_NAME_SIZE];
    strcpy(info + size, dir);
    size += strlen(dir) + 1;

    unsigned char offset = rand() % PAYLOAD_MAX_BLOCKS;
    info[size] = offset;
    size++;

    _syscall(info, size);

    char error;
    read(syscallReturnFd, &error, sizeof(char));
    
    if (error)
        printf("Add dir error at %s, new dir: %s\n", path, dir);
    else
        printf("Add dir success at %s, new dir: %s\n", path, dir);
}

void callRemove()
{
    char info[MAX_SYSCALL_INFO_SIZE];
    int size = 0;

    info[size] = RM;
    size++;

    const char* path = RM_PATHS[rand() % RM_PATH_SIZE];
    strcpy(info + size, path);
    size += strlen(path) + 1;

    _syscall(info, size);

    char error;
    read(syscallReturnFd, &error, sizeof(char));
    
    if (error)
        printf("Remove error at %s\n", path);
    else
        printf("Remove success at %s\n", path);
}

void callList()
{
    char info[MAX_SYSCALL_INFO_SIZE];
    int size = 0;

    info[size] = LS;
    size++;

    const char* path = LS_PATHS[rand() % LS_PATH_SIZE];
    strcpy(info + size, path);
    size += strlen(path) + 1;

    _syscall(info, size);

    char error;
    read(syscallReturnFd, &error, sizeof(char));
    
    if (error)
        printf("List error at %s\n", path);
    else
    {
        int total;
        int dirStarts[MAX_LIST_DIRS];
        char dirs[MAX_DIR_LEN * MAX_LIST_DIRS];

        read(syscallReturnFd, &total, sizeof(int));

        int totalSent = total < MAX_LIST_DIRS ? total : MAX_LIST_DIRS;
        read(syscallReturnFd, dirStarts, sizeof(int) * totalSent);

        int currentDir = 0;
        int currentChar = 0;
        while (currentDir < totalSent)
        {
            read(syscallReturnFd, dirs + currentChar, sizeof(char));
            if (dirs[currentChar] == '\0')
                currentDir++;
            currentChar++;
        }

        printf("List success at %s, total: %d, dirs: ", path, total);
        for (int i = 0; i < totalSent; i++)
            printf("%s ", dirs + dirStarts[i]);
        printf("\n");
    }
}

void callFinish()
{
    char info[1] = {
        FINISH
    };
    _syscall(info, 1);
}
