#define _BSD_SOURCE
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include "ds_err.h"

int main(int argc, char *argv[])
{
    struct stat st;

    if (argc != 2) {
        err_quit("usage: %s pathname", argv[0]);
    }
    if (stat(argv[1], &st) < 0) {
        err_sys("stat error");
    }

    printf("dev: [%lu] = [%d:%d]\n", st.st_dev, major(st.st_dev), minor(st.st_dev));
    printf("uid: %u, gid: %u\n", st.st_uid, st.st_gid);
    printf("inode: %lu\n", st.st_ino);
    printf("nlinks: %lu\n", st.st_nlink);
    printf("file size: %lu\n", st.st_size);
    printf("file type: %s\n", S_ISREG(st.st_mode) ? "regular" : "not regular");
    printf("file blocks: %d\n", st.st_blocks);
    printf("file blocksize: %d\n", st.st_blksize);

    return 0;
}
