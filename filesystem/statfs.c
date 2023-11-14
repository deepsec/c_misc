#include <sys/statvfs.h>
#include <stdio.h>
#include "ds_err.h"

int main(int argc, char *argv[])
{
    if (argc != 2) {
        err_quit("usage: %s path", argv[0]);
    }

    struct statvfs sv;
    if (statvfs(argv[1], &sv) < 0) {
        err_sys("statvfs error");
    }

    printf("block size: %lu\n", sv.f_bsize);
    printf("fragment size: %lu\n", sv.f_frsize);
    printf("blocks: %lu\n", sv.f_blocks);
    printf("free blocks: %lu\n", sv.f_bfree);
    printf("available blocks: %lu\n", sv.f_bavail);
    printf("inodes: %lu\n", sv.f_files);
    printf("free inodes: %lu\n", sv.f_ffree);
    printf("available inodes: %lu\n", sv.f_favail);
    printf("file system id: %lu\n", sv.f_fsid);
    printf("mount flags: %lu\n", sv.f_flag);
    printf("maximum filename length: %lu\n", sv.f_namemax);

    return 0;
}
