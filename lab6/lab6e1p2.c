#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#define FIFO "fifoEx1"

int main()
{
    int fpFIFO = open(FIFO, O_WRONLY);
    if (fpFIFO < 0)
    {
        printf("Erro abrindo FIFO\n");
        return 1;
    }

    int stdin = dup(STDIN_FILENO);

    while (1)
    {
        char c;
        if (read(stdin, &c, sizeof(char)) > 0)
            write(fpFIFO, &c, sizeof(char));
    }
}
