#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>
#include <errno.h>
#include <signal.h>

#define FIFO_NAME "myfifo"
#define HEADER_DATA "DATA:"
#define HEADER_SIGN "SIGN:"
#define NAME_FILE_TXT "Log.txt"
#define NAME_SIGN_TXT "Sign.txt"
#define SIZE_OF_HEADER 5
#define BUFFER_SIZE 300

FILE *fileLog;
FILE *fileSignal;

void sigint_handler(int sig)
{
    /* Handler to close de files */
    write(1, "se presiono ctrl+c!!\n\r", 21);
    write(1, "Cierro archivos txt\n\r", 20);
    fclose(fileLog);
    fclose(fileSignal);
}

int main(void)
{
    char inputBuffer[BUFFER_SIZE];
    int32_t bytesRead, returnCode, fd;

    struct sigaction sa;

    sa.sa_handler = sigint_handler;
    sa.sa_flags = 0; //SA_RESTART;
    sigemptyset(&sa.sa_mask);

    if ((sigaction(SIGINT, &sa, NULL) == -1))
    {
        perror("sigaction");
    }

    /* Call umask() before mknod to set the permissions to file correctly */
    /* $ umask = 0002 defalt mask */
    umask(0);

    /* Create named fifo. -1 means already exists so no action if already exists */
    if ((returnCode = mknod(FIFO_NAME, S_IFIFO | 0666, 0)) < -1)
    {
        printf("Error creating named fifo: %d\n", returnCode);
        perror("mknod");
    }

    /* Open named fifo. Blocks until other process opens it */
    printf("waiting for writers...\n");
    if ((fd = open(FIFO_NAME, O_RDONLY)) < 0)
    {
        printf("Error opening named fifo file: %d\n", fd);
        perror("open");
    }

    /* Open file .txt. Blocks until other process opens it */
    printf("Opening file txt...\n");
    if ((fileLog = fopen(NAME_FILE_TXT, "a+")) == NULL)
    {
        perror("fopen");
    }

    /* Open file .txt. Blocks until other process opens it */
    printf("Opening file signals txt...\n");
    if ((fileSignal = fopen(NAME_SIGN_TXT, "a+")) == NULL)
    {
        perror("fopen");
    }

    /* open syscalls returned without error -> other process attached to named fifo */
    printf("got a writer\n");

    /* Loop until read syscall returns a value <= 0 */
    do
    {
        /* read data into local buffer */
        if ((bytesRead = read(fd, inputBuffer, BUFFER_SIZE)) == -1)
        {
            perror("read");
        }
        else
        {
            /* last character of the bytes read */
            inputBuffer[bytesRead] = '\0';

            /* if the first charactar is '\0', don't write the files */
            if (inputBuffer[0] != '\0')
            {
                if (strncmp(HEADER_DATA, inputBuffer, SIZE_OF_HEADER) == 0)
                {
                    fputs(inputBuffer, fileLog);
                    printf("reader: read %d bytes: \"%s\"\n", bytesRead, inputBuffer);
                }
                else if (strncmp(HEADER_SIGN, inputBuffer, SIZE_OF_HEADER) == 0)
                {
                    strcat(inputBuffer, "\n");
                    fputs(inputBuffer, fileSignal);
                    printf("reader: read %d bytes: \"%s\"\n", bytesRead, inputBuffer);
                }
            }
        }
    } while (bytesRead > 0);

    return 0;
}
