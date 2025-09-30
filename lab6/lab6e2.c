#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <wait.h>

#define FIFO "fifoEx2"

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

    for (int i = 0; i < 2; i++)
    {
        if (fork() == 0)
        {
            int fpFIFO = open(FIFO, O_WRONLY);
            if (fpFIFO < 0)
            {
                printf("Erro abrindo FIFO para escrita\n");
                return 1;
            }
            
            char str[] = "0abc";
            str[0] = i + '0';
            write(fpFIFO, str, sizeof(char) * 5);
            
            close(fpFIFO);

            return 0;
        }
    }

    int fpFIFO = open(FIFO, O_RDONLY);
    if (fpFIFO < 0)
    {
        printf("Erro abrindo FIFO para leitura\n");
        return 1;
    }

    for (int i = 0; i < 2; i++)
        waitpid(-1, NULL, 0);

    char c;
    while (read(fpFIFO, &c, sizeof(char)) > 0)
        printf("%c", c);
    printf("\nclose\n");

    close(fpFIFO);

    return 0;
}
