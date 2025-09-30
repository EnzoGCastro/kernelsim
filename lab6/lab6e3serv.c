#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#define FIFO_IN "fifoEx3In"
#define FIFO_OUT "fifoEx3Out"

int main()
{
    if (access(FIFO_IN, F_OK) == -1)
    {
        if (mkfifo(FIFO_IN, S_IRUSR | S_IWUSR) != 0)
        {
            printf("Erro criando FIFO\n");
            return 1;
        }
    }

    if (access(FIFO_OUT, F_OK) == -1)
    {
        if (mkfifo(FIFO_OUT, S_IRUSR | S_IWUSR) != 0)
        {
            printf("Erro criando FIFO\n");
            return 1;
        }
    }

    int fpFIFOIn = open(FIFO, O_WRONLY);
    if (fpFIFOIn < 0)
    {
        printf("Erro abrindo FIFO\n");
        return 1;
    }

    int fpFIFOOut = open(FIFO, O_RDONLY);
    if (fpFIFOOut < 0)
    {
        printf("Erro abrindo FIFO\n");
        return 1;
    }

    while (1)
    {
        char c;
        if (read(fpFIFOIn, &c, sizeof(char)) > 0)
        {
            if (c >= 'a' && c <= 'z')
                c += 'A' - 'a';
            write(fpFIFOOut, &c, sizeof(char));
        }
    }
    
    return 0;
}
