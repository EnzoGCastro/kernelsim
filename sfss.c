#include "sfss.h"

#include "udpserver.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/dir.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <unistd.h>

int PathExtendCwd(char* path, int ax, char* out);
void PathAppend(char* path, char* str);
void PathReturn(char* path);

int Write(char* path, char* payload, int size, int offset);
int Read(char* path, char* out, int offset);

int AddDir(char* path, char* new);
int Remove(char* path);
int RemoveRecursive(char* path);

int List(char* path, char* out, int* outStarts, int* total);

int main()
{
    char fullPath[MAXPATHLEN];

    PathExtendCwd("", -1, fullPath);
    
    mkdir(fullPath, 0755);

    char Ax[3];
    Ax[0] = 'A';
    Ax[2] = '\0';
    for (int i = 0; i < 5; i++)
    {
        Ax[1] = '1' + i;
        PathAppend(fullPath, Ax);
        mkdir(fullPath, 0755);
        PathReturn(fullPath);
    }

    SetupUdpServer(UDP_PORT);

    char message[MAX_UDP_MESSAGE_SIZE];
    char messagePath[MAX_PATH_LEN];
    char messagePayload[PAYLOAD_BLOCK_SIZE * PAYLOAD_MAX_BLOCKS];

    char returnMessage[MAX_UDP_MESSAGE_SIZE];

    char* messageP;

    char offset;
    char error;
    char outList[MAX_DIR_LEN * MAX_LIST_DIRS];
    int outListStarts[MAX_LIST_DIRS];
    int totalList;

    while (ReceiveMessage(message, MAX_UDP_MESSAGE_SIZE) > 0)
    {
        messageP = message;

        int returnMessageSize = 0;

        char id = *messageP;
        messageP++;

        char ax = *messageP;
        messageP++;

        char instruction = *messageP;
        messageP++;

        strcpy(messagePath, messageP);
        messageP += strlen(messagePath) + 1;

        PathExtendCwd(messagePath, ax, fullPath);

        // Write return message header
        returnMessage[returnMessageSize] = id;
        returnMessageSize++;
        returnMessage[returnMessageSize] = ax;
        returnMessageSize++;
        returnMessage[returnMessageSize] = instruction;
        returnMessageSize++;

        switch (instruction)
        {
        case SFSS_WRITE:
            char payloadSize = *messageP;
            messageP++;

            for (int i = 0; i < payloadSize * PAYLOAD_BLOCK_SIZE; i++)
            {
                messagePayload[i] = *messageP;
                messageP++;
            }

            offset = *messageP;
            messageP++;

            error = Write(fullPath, messagePayload, payloadSize, offset);

            returnMessage[returnMessageSize] = error;
            returnMessageSize++;
            
            break;
        
        case SFSS_READ:
            offset = *messageP;
            messageP++;

            error = Read(fullPath, messagePayload, offset);

            returnMessage[returnMessageSize] = error;
            returnMessageSize++;

            if (!error)
                for (int i = 0; i < PAYLOAD_BLOCK_SIZE; i++)
                {
                    returnMessage[returnMessageSize] = messagePayload[i];
                    returnMessageSize++;
                }

            break;

        case SFSS_ADDDIR:
            strcpy(messagePayload, messageP); // dir
            messageP += strlen(messagePayload) + 1;

            error = AddDir(fullPath, messagePayload);

            returnMessage[returnMessageSize] = error;
            returnMessageSize++;

            break;

        case SFSS_REMOVE:
            error = Remove(fullPath);

            returnMessage[returnMessageSize] = error;
            returnMessageSize++;

            break;

        case SFSS_LIST:
            error = List(fullPath, outList, outListStarts, &totalList);

            returnMessage[returnMessageSize] = error;
            returnMessageSize++;

            if (!error)
            {
                memcpy(returnMessage + returnMessageSize, &totalList, sizeof(int));
                returnMessageSize += sizeof(int);

                int totalSent = totalList < MAX_LIST_DIRS ? totalList : MAX_LIST_DIRS;

                memcpy(returnMessage + returnMessageSize, outListStarts, sizeof(int) * totalSent);
                returnMessageSize += sizeof(int) * totalSent;

                char* out = outList;
                for (int i = 0; i < totalSent; i++)
                {
                    strcpy(returnMessage + returnMessageSize, out);
                    returnMessageSize += strlen(out) + 1;
                    out += strlen(out) + 1;
                }
            }

            break;
        }

        SendMessage(returnMessage, returnMessageSize);
    }
}

int PathExtendCwd(char* path, int ax, char* out)
{
    if (getcwd(out, MAXPATHLEN) == NULL)
        return 1;
    
    while(*out)
        out++;

    out[0] = '/';
    out++;

    strcpy(out, SFSS_ROOT_PATH);
    out += strlen(SFSS_ROOT_PATH);

    if (ax != -1)
    {
        out[0] = '/';
        out[1] = 'A';
        out[2] = '1' + ax;
        out += 3;
    }

    strcpy(out, path);

    return 0;
}

void PathAppend(char* path, char* str)
{
    int j = -1;
    for (int i = 0; i < MAXPATHLEN; i++)
    {
        if (j == -1)
        {
            if (path[i] == '\0')
            {
                path[i] = '/';
                j = 0;
            }
        }
        else
        {
            path[i] = str[j];
            if (str[j] == '\0')
                break;
            j++;
        }
    }
}

void PathReturn(char* path)
{
    int end = -1;
    for (int i = 0; i < MAXPATHLEN; i++)
        if (path[i] == '\0')
        {
            end = i;
            break;
        }

    for (int i = end; i >= 0; i--)
        if (path[i] == '/')
        {
            path[i] = '\0';
            break;
        }
}

int Write(char* path, char* payload, int size, int offset)
{
    FILE* file = fopen(path, "a");
    if (file == NULL)
        return 1;

    struct stat st;
    if (stat(path, &st))
        return 1;

    if (st.st_size < offset * PAYLOAD_BLOCK_SIZE)
    {
        char whitespaces[PAYLOAD_BLOCK_SIZE];
        memset(whitespaces, ' ', PAYLOAD_BLOCK_SIZE);
        int writes = offset - (st.st_size / PAYLOAD_BLOCK_SIZE);
        for (int i = 0; i < writes; i++)
            fwrite(whitespaces, sizeof(char), PAYLOAD_BLOCK_SIZE, file);
    }
    
    if (fseek(file, offset * PAYLOAD_BLOCK_SIZE, SEEK_SET))
        return 1;

    fwrite(payload, sizeof(char), size * PAYLOAD_BLOCK_SIZE, file);
    fclose(file);

    return 0;
}

int Read(char* path, char* out, int offset)
{
    FILE* file = fopen(path, "r");
    if (file == NULL)
        return 1;
    
    struct stat st;
    if (stat(path, &st))
        return 1;

    if (st.st_size < (offset + 1) * PAYLOAD_BLOCK_SIZE)
        return 1;

    if (fseek(file, offset * PAYLOAD_BLOCK_SIZE, SEEK_SET))
        return 1;

    if (fread(out, 1, PAYLOAD_BLOCK_SIZE, file) < PAYLOAD_BLOCK_SIZE)
        return 1;

    return 0;
}

int AddDir(char* path, char* new)
{
    struct stat st;
    if (stat(path, &st) || !S_ISDIR(st.st_mode))
        return 1;

    PathAppend(path, new);
    if (mkdir(path, 0755))
        return 1;
    PathReturn(path);

    return 0;
}

int Remove(char* path)
{
    struct stat st;
    if (stat(path, &st))
        return 1;

    if (S_ISDIR(st.st_mode))
        return RemoveRecursive(path);

    remove(path);

    return 0;
}

int RemoveRecursive(char* path)
{
    struct dirent** files;
    int count = scandir(path, &files, NULL, alphasort);

    if (count == -1) 
        return 1;

    for (int i = 2; i < count; i++)
    {
        PathAppend(path, files[i]->d_name);
        
        if (files[i]->d_type == DT_DIR)
        {
            if (RemoveRecursive(path))
                return 1;
        }
        else if (files[i]->d_type == DT_REG)
            remove(path);

        PathReturn(path);
    }

    for (int i = 0; i < count; i++)
        free(files[i]);
    free(files);

    remove(path);

    return 0;
}

int List(char* path, char* out, int* outStarts, int* total)
{
    struct dirent** files;
    int count = scandir(path, &files, NULL, alphasort);

    if (count == -1)
        return 1;

    int c = 0;
    for (int i = 2; i < count && i < MAX_LIST_DIRS + 2; i++)
    {
        outStarts[i - 2] = c;
        strcpy(out, files[i]->d_name);
        int len = strlen(files[i]->d_name);
        out += len + 1;
        c += len + 1;
    }

    for (int i = 0; i < count; i++)
        free(files[i]);
    free(files);

    *total = count - 2;

    return 0;
}
