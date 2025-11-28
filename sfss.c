#include "sfss.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/dir.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <unistd.h>

int PathExtendCwd(char* path, char* out);
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
    int result;
    PathExtendCwd("", fullPath);
    AddDir(fullPath, "deletemefolder");
    AddDir(fullPath, "deleteme");
    
    PathExtendCwd("deletemefolder", fullPath);
    AddDir(fullPath, "deletemeinside");

    PathExtendCwd("deleteme.txt", fullPath);
    result = Write(fullPath, "1234567890123456", 1, 2);
    printf("Write result %d\n", result);

    char r[17];
    r[16] = '\0';
    result = Read(fullPath, r, 0);
    printf("Read result %d\n", result);
    printf("%s\n", r);
    result = Read(fullPath, r, 1);
    printf("Read result %d\n", result);
    printf("%s\n", r);
    result = Read(fullPath, r, 2);
    printf("Read result %d\n", result);
    printf("%s\n", r);
    result = Read(fullPath, r, 3);
    printf("Read result %d\n", result);
    printf("%s\n", r);

    PathExtendCwd("", fullPath);
    char out[MAX_DIR_LEN * MAX_LIST_DIRS];
    int outStarts[MAX_LIST_DIRS];
    int total;
    List(fullPath, out, outStarts, &total);
    for (int i = 0; i < total; i++)
        printf("%d: %s\n", outStarts[i], out + outStarts[i]);
    printf("%d\n", total);

    /*
    result = Remove(fullPath);
    printf("Operation: Remove existing (recursive). Result: %d\n", result);
    result = Remove(fullPath);
    printf("Operation: Remove inexistent. Result: %d\n", result);
    
    PathExtendCwd("deleteme", fullPath);

    result = Remove(fullPath);
    printf("Operation: Remove existing (non recursive). Result: %d\n", result);
    result = Remove(fullPath);
    printf("Operation: Remove inexistent. Result: %d\n", result);
    */

}

int PathExtendCwd(char* path, char* out)
{
    if (getcwd(out, MAXPATHLEN) == NULL)
        return 1;
    
    while(*out)
        out++;
    
    strcpy(out, SFSS_ROOT_PATH);
    out += strlen(SFSS_ROOT_PATH);

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
