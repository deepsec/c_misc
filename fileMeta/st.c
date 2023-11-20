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
    printf("file size: %lu\n", st.st_size);

    return 0;
}
