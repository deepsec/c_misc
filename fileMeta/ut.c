#define _BSD_SOURCE
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <utime.h>
#include <time.h>
#include "ds_err.h"

int main(int argc, char *argv[])
{
    struct stat st, stnew;
    struct utimbuf  utb;

    if (argc != 2) {
        err_quit("usage: %s pathname", argv[0]);
    }

    if (stat(argv[1], &st) < 0) {
        err_sys("stat error");
    }
    printf("Last access: %s", ctime(&st.st_atime));
    printf("Last modification: %s", ctime(&st.st_mtime));
    printf("Last status change: %s", ctime(&st.st_ctime));
    printf("Last status change(ns): %lu.%lu\n", st.st_ctime, st.st_ctim.tv_nsec);

    utb.actime = st.st_atime;
    utb.modtime = st.st_atime;
    if(utime(argv[1], &utb) == -1) {
        err_sys("utime error");
    }
    if (stat(argv[1], &stnew) < 0) {
        err_sys("stat error");
    }
    printf("*****************************************\n");
    printf("Last access: %s", ctime(&stnew.st_atime));
    printf("Last modification: %s", ctime(&stnew.st_mtime));
    printf("Last status change: %s", ctime(&stnew.st_ctime));
    printf("Last status change(ns): %lu.%lu\n", stnew.st_ctime, stnew.st_ctim.tv_nsec);

    return 0;
}
