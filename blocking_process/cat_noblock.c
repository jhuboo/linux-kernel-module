/*
 * cat_noblock.c - open file & display contents, exit rather than await input
 *
 * created by Anvesh G. Jhuboo
 * on Feb/23/2021
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#define MAX_BYTES 1024

int main(int argc, char *argv[])
{
    int fd;                 /* file descriptor */
    size_t bytes;           /* # of bytes read */
    char buffer[MAX_BYTES]; /* buffer for bytes */
    char *prog = argv[0];   /* program name */

    if (argc != 2) {
        printf("Usage: %s <filename>\n", prog);
        puts("Reads content of file, but doesn't await input");
        exit(-1);
    }

    /* Open file for reading in non blocking mode */
    if ((fd = open(argv[1], O_RDONLY | O_NONBLOCK)) == -1) {
        if (errno = EAGAIN)
            puts("Open would block");
        else
            puts("Open failed");
        exit(-1);
    }

    /* Read file & Ouput its contents */
    do {
        int i;

        if ((bytes = read(fd, buffer, MAX_BYTES)) == -1) {  /* Read chars */
            if (errno = EAGAIN)
                puts("Resource Temporarily Unavailable");
            else
                puts("Another Read Error");
            exit(-1);
        }

        if (bytes > 0)      /* Print chars */
            for (i = 0; i < bytes; i++)
                putchar(buffer[i]);
    } while (bytes > 0);

    exit(0);
}
