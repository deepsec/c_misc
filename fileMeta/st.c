#define _BSD_SOURCE
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include "ds_err.h"

static void showFileType(const struct stat *stp)
{
    switch (stp->st_mode & S_IFMT) {
    case S_IFREG:   printf("regular file\n");           break;
    case S_IFDIR:   printf("directory\n");              break;
    case S_IFCHR:   printf("character device\n");       break;
    case S_IFBLK:   printf("block device\n");           break;
    case S_IFLNK:   printf("symbolic (soft) link\n");   break;
    case S_IFIFO:   printf("FIFO or pipe\n");           break;
    case S_IFSOCK:  printf("socket\n");                 break;
    default:        printf("unknown file type!\n");     break;
    }
}


int main(int argc, char *argv[])
{
    struct stat st;

    if (argc != 2) {
        err_quit("usage: %s pathname", argv[0]);
    }
    if (stat(argv[1], &st) < 0) {
        err_sys("stat error");
    }
    showFileType(&st);
    if (S_ISCHR(st.st_mode) || S_ISBLK(st.st_mode)) {
        printf("dev: [%lu] = [%d:%d]\n", st.st_rdev, major(st.st_rdev), minor(st.st_rdev));
    } else {
        printf("dev: [%lu] = [%d:%d]\n", st.st_dev, major(st.st_dev), minor(st.st_dev));
    }
    printf("uid: %u, gid: %u\n", st.st_uid, st.st_gid);
    printf("inode: %lu\n", st.st_ino);
    printf("nlinks: %lu\n", st.st_nlink);
    printf("file size: %lu\n", st.st_size);
    printf("file blocks(512Bytes): %ld\n", st.st_blocks);
    printf("file blocksize: %ld\n", st.st_blksize);
    printf("Last access: %s", ctime(&st.st_atime));
    printf("Last modification: %s", ctime(&st.st_mtime));
    printf("Last status change: %s", ctime(&st.st_ctime));

    return 0;
}
