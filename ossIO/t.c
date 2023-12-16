#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include "ds_syscall.h"
#include "ds_common.h"
#include "ds_err.h"


int main(int argc, char *argv[])
{
	long  ino_free, ino_total, blk_free, blk_total, blk_size;
	if (argc != 2) {
		err_quit("USAGE: %s pathname", argv[0]);
	}

	if (fs_info(argv[1], &ino_free, &ino_total, &blk_free, &blk_total, &blk_size) < 0) {
		err_sys("fs_info() error");
	}
	printf("ino_free: %ld,  ino_total: %ld,  blk_free: %ld, blk_total: %ld, blk_size: %ld\n", 
			ino_free, ino_total, blk_free, blk_total, blk_size);

	printf("%2.2f\n", ((double)(blk_free)/(double)(blk_total)));

	return 0;
}
