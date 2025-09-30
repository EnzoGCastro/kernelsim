#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#define FIFO_IN "fifoEx3In"
#define FIFO_OUT "fifoEx3Out"

int main()
{
     int fpFIFOIn = open(FIFO, O_RDONLY);
    if (fpFIFOIn < 0)
    {
        printf("Erro abrindo FIFO\n");
        return 1;
    }

    int fpFIFOOut = open(FIFO, O_WRONLY);
    if (fpFIFOOut < 0)
    {
        printf("Erro abrindo FIFO\n");
        return 1;
    }

    char str[] = "abc";
    write(fpFIFOOut, str, sizeof(char) * 5);
    
    while (1)
    {
        char c;
        if (read(fpFIFOIn, &c, sizeof(char)) > 0)
            printf("%c", c);
    }

    return 0;
}
