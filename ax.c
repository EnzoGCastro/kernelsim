#include "kernel.h"

AppData* data;
int syscallFd; // pipe para dados do syscall
int myPid;

const int FILE_PATH_SIZE = 4;
const char* const FILE_PATHS[] = {
    "f1.txt",
    "f2.txt",
    "dir/f3.txt",
    "dir/dir/f4.txt"
};

const int AD_PATH_SIZE = 2;
const char* const AD_PATHS[] = {
    "",
    "dir"
};
const int AD_NAME_SIZE = 1;
const char* const AD_NAMES[] = {
    "dir"
};

const int RM_PATH_SIZE = 6;
const char* const RM_PATHS[] = {
    "f1.txt",
    "f2.txt",
    "dir",
    "dir/f3.txt",
    "dir/dir",
    "dir/dir/f4.txt"
};

const int LS_PATH_SIZE = 3;
const char* const LS_PATHS[] = {
    "",
    "dir",
    "dir/dir"
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

            d = d % 5;
            switch(d)
            {
            case 0:
                callWrite();
                break;
            case 1:
                callRead();
                break;
            case 2:
                callAdd();
                break;
            case 3:
                callRemove();
                break;
            case 4:
                callList();
                break;
            }
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

    unsigned  char payloadSize = ((rand() % PAYLOAD_MAX_BLOCKS) + 1) * PAYLOAD_BLOCK_SIZE;
    info[size] = payloadSize;
    size++;

    char payload[PAYLOAD_BLOCK_SIZE * PAYLOAD_MAX_BLOCKS];
    int p = rand() % 10 + '0';
    for (int i = 0; i < payloadSize; i++)
    {
        payload[i] = p;
        info[size] = payload[i];
        size++;
    }

    unsigned char offset = (rand() % PAYLOAD_MAX_BLOCKS) * PAYLOAD_BLOCK_SIZE;
    info[size] = offset;
    size++;
    
    _syscall(info, size);
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

    int fd[2];
    pipe(fd);

    char* charFd = (char*) &(fd[1]);
    for (int i = 0; i < 4; i++)
    {
        info[size] = charFd[i];
        size++;
    }

    unsigned char offset = (rand() % PAYLOAD_MAX_BLOCKS) * PAYLOAD_BLOCK_SIZE;
    info[size] = offset;
    size++;
    
    _syscall(info, size);

    // ler fd
    //char buffer[16];
    //read(fd[0], &buffer, 16);

    close(fd[0]);
    close(fd[1]);
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

    unsigned char offset = (rand() % PAYLOAD_MAX_BLOCKS) * PAYLOAD_BLOCK_SIZE;
    info[size] = offset;
    size++;

    _syscall(info, size);
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

    int fd[2];
    pipe(fd);

    char* charFd = (char*) &(fd[1]);
    for (int i = 0; i < 4; i++)
    {
        info[size] = charFd[i];
        size++;
    }

    _syscall(info, size);
}

void callFinish()
{
    char info[1] = {
        FINISH
    };
    _syscall(info, 1);
}
