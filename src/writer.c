#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>
#include <signal.h>

#define FIFO_NAME "myfifo"
#define BUFFER_SIZE 300
#define HEADER_DATA "DATA:"
#define MSG_SIGN1 "SIGN:1"
#define MSG_SIGN2 "SIGN:2"
#define SIZE_MSG (BUFFER_SIZE - strlen(HEADER_DATA))
#define END_OF_DATA "\n"

int32_t fd;

void sigusrn_handler(int sig)
{
    uint32_t bytesWrote;
    if (sig == SIGUSR1)
    {
        if ((bytesWrote = write(fd, MSG_SIGN1, strlen(MSG_SIGN1))) == -1)
        {
            perror("write");
        }
    }
    else if (sig == SIGUSR2)
    {
        if ((bytesWrote = write(fd, MSG_SIGN2, strlen(MSG_SIGN2))) == -1)
        {
            perror("write");
        }
    }
}

int main(void)
{
    uint32_t bytesWrote;
    char outputBuffer[SIZE_MSG];
    char bufferToSend[BUFFER_SIZE];
    int32_t returnCode;
    struct sigaction sa;

    /* Initialize de struct sigaction */
    sa.sa_handler = sigusrn_handler;
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);

    if ((sigaction(/* SIGINT */ SIGUSR1, &sa, NULL) == -1) || (sigaction(/* SIGINT */ SIGUSR2, &sa, NULL) == -1))
    {
        perror("sigaction");
        exit(1);
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
    printf("waiting for readers...\n");
    if ((fd = open(FIFO_NAME, O_WRONLY)) < 0)
    {
        printf("Error opening named fifo file: %d\n", fd);
        perror("open");
    }

    /* open syscalls returned without error -> other process attached to named fifo */
    printf("got a reader--type some stuff\n");

    /* Loop forever */
    while (1)
    {

        /* Get some text from console */
        if (fgets(outputBuffer, SIZE_MSG, stdin) == NULL)
        {
            perror("fgets");
        }

        /* Concatenate DATA:'outputBuffer\n' from writer for send to reader */
        strcat(bufferToSend, HEADER_DATA);
        strcat(bufferToSend, outputBuffer);
        strcat(bufferToSend, END_OF_DATA);

        /* Write buffer to named fifo. Strlen - 1 to avoid sending \n char */
        if ((bytesWrote = write(fd, bufferToSend, strlen(bufferToSend) - 1)) == -1)
        {
            perror("write");
        }
        else
        {
            printf("writer: wrote %d bytes\n", bytesWrote);
        }
        /* Clear bufferToSend */
        bufferToSend[0] = '\0';
    }
    return 0;
}
