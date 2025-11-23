#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define DIR_T 0
#define FILE_T 1

typedef struct fileDir
{
    char name[256];
    int type;
    struct fileDir* next;
    struct fileDir* child;
} FileDir;

FileDir* first = NULL;

FileDir* Find(FileDir* parent, char* name);
int Insert(char* path, char* name, int type);
int Erase(char* path);
void EraseRecursive(FileDir* current, int initial);

int main()
{
    int result;
    result = Insert("", "a1", DIR_T);
    printf("Inserting a1 at root, result %d\n", result);
    
    result = Insert("", "a1", DIR_T);
    printf("Inserting a1 at root, result %d\n", result);

    result = Insert("", "a2", DIR_T);
    printf("Inserting a2 at root, result %d\n", result);

    result = Insert("a2", "ina2", DIR_T);
    printf("Inserting ina2 at a2, result %d\n", result);

    result = Insert("a2/ina2", "inina2", DIR_T);
    printf("Inserting ina2 at a2, result %d\n", result);

    result = Insert("", "test.txt", FILE_T);
    printf("Inserting test.txt at root, result %d\n", result);

    result = Insert("", "test.txt", FILE_T);
    printf("Inserting test.txt at root, result %d\n", result);

    result = Insert("a2/ina2", "test.txt", FILE_T);
    printf("Inserting test.txt at a2/ina2, result %d\n", result);

    result = Erase("test.txt");
    printf("Erasing test.txt at root, result %d\n", result);

    result = Insert("", "test.txt", FILE_T);
    printf("Inserting test.txt at root, result %d\n", result);

    result = Erase("a2");
    printf("Erasing a2 at root, result %d\n", result);

    result = Insert("a2", "ina2", DIR_T);
    printf("Inserting ina2 at a2, result %d\n", result);
}

// Retorna o primeiro filho de parent com name ou NULL caso nao tenha
// Caso parent seja NULL, assume que e o root e o primeiro filho seria first
FileDir* Find(FileDir* parent, char* name)
{
    FileDir* current = parent != NULL ? parent->child : first;
    while (current != NULL && strcmp(current->name, name))
        current = current->next;
    return current;
}

int Insert(char* path, char* name, int type)
{
    if (strlen(name) == 0)
        return 1;

    FileDir* new = (FileDir*)malloc(sizeof(FileDir));
    new->type = type;
    strcpy(new->name, name);

    char dir[256];
    FileDir* current = NULL;

    while (1)
    {
        dir[0] = '\0';
        //Constroi dir ate o primeiro / encontrado ou se chegar ao final do path
        for (int i = 0; path[0] != '/' && path[0] != '\0'; i++)
        {
            dir[i] = path[0];
            dir[i + 1] = '\0';
            path++;
        }
        
        // Caso path seja o root, inserimos no lugar do first o new
        if (current == NULL && path[0] == '\0' && strlen(dir) == 0)
        {
            if (Find(current, new->name) != NULL)
                return 1; // Ja existe algo com esse nome
            
            new->next = first;
            first = new;
            printf("Inserted %s\n", new->name);
            return 0;
        }

        current = Find(current, dir);
        if (current == NULL)
            return 1;

        // Se proximo char de path for '\0' insere next como child de current e finaliza a recursao
        if (path[0] == '\0')
        {
            if (current->type == FILE_T)
                return 1;

            if (Find(current, new->name) != NULL)
                return 1; // Ja existe algo com esse nome

            new->next = current->child;
            current->child = new;
            printf("Inserted %s\n", new->name);
            return 0;
        }

        path++;
        printf("Insert entered %s\n", current->name);
    }

    return 1;
}

int Erase(char* path)
{
    char dir[256];
    FileDir* current = first;
    FileDir* parent = NULL;
    FileDir* prev = NULL;
    
    while(1)
    {
        dir[0] = '\0';
        //Constroi dir ate o primeiro / encontrado ou se chegar ao final do path
        for (int i = 0; path[0] != '/' && path[0] != '\0'; i++)
        {
            dir[i] = path[0];
            dir[i + 1] = '\0';
            path++;
        }
        
        while (current != NULL && strcmp(current->name, dir))
        {
            prev = current;
            current = prev->next;
        }
        
        if (current == NULL)
            return 1;

        if (path[0] == '\0')
            break;
        
        parent = current;
        current = parent->child;
        prev = parent;
        path++;
    }
    
    if (prev == NULL)
        first = current->next;
    else if (parent == prev)
        parent->child = current->next;
    else
        prev->next = current->next;
    
    EraseRecursive(current, 1);

    return 0;
}

void EraseRecursive(FileDir* current, int initial)
{
    if (current == NULL)
        return;

    if (!initial)
        EraseRecursive(current->next, 0);
    EraseRecursive(current->child, 0);

    printf("Erased %s\n", current->name);
    free(current);
}
