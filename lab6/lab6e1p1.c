#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#define FIFO "fifoEx1"

int main()
{
    if (access(FIFO, F_OK) == -1)
    {
        if (mkfifo(FIFO, S_IRUSR | S_IWUSR) != 0)
        {
            printf("Erro criando FIFO\n");
            return 1;
        }
    }

    int fpFIFO = open(FIFO, O_RDONLY);
    if (fpFIFO < 0)
    {
        printf("Erro abrindo FIFO\n");
        return 1;
    }

    int stdout = dup(STDOUT_FILENO);

    while (1)
    {
        char c;
        if (read(fpFIFO, &c, sizeof(char)) > 0)
            write(stdout, &c, sizeof(char));
    }
}
