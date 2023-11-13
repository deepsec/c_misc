#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "ds_err.h"

#ifndef BUF_SIZE
#define BUF_SIZE 1024
#endif

int main(int argc, char *argv[])
{
    int inputFd, outputFd, openFlags;
    mode_t filePerms;
    ssize_t numRead;
    char    buf[BUF_SIZE];

    if (argc != 3 || strcmp(argv[1], "--help") == 0) {
        err_quit("USAGE: %s old-file new-file", argv[0]);
    }

    inputFd = open(argv[1], O_RDONLY);
    if (inputFd == -1) {
        err_sys("open file['%s'] error", argv[1]);
    }

    openFlags = O_CREAT | O_WRONLY | O_TRUNC;
    //filePerms = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;  // rw-rw-rw-
    outputFd = open(argv[2], openFlags, 0644);
    if (outputFd == -1) {
        err_sys("opening file['%s'] error", argv[2]);
    }

    for (;;) {
        numRead = read(inputFd, buf, BUF_SIZE);
        if (numRead == 0) {
            break;
        }
        if (numRead == -1) {
            err_sys("read error");
        }
        if (write(outputFd, buf, numRead) != numRead) {
            err_sys("couldn't write whole buffer");
        }
    } 
    
    if (fsync(outputFd) == -1) {
        err_sys("fsync error");
    }

    if (close(inputFd) == -1) {
        err_sys("close inputFd['%d'] error", inputFd);
    };
     if (close(outputFd) == -1) {
        err_sys("close outputFd['%d'] error", outputFd);
    };

#include <stdio.h>
    printf("BUFSIZ[%d]\n", BUFSIZ);
    return 0;
}
