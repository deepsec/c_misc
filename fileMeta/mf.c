#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "ds_err.h"

int main(int argc, char *argv[])
{
    unsigned long size, count, i, j, k;
    char *mem_ptr, *ptr;
    char content[1024] = "0123456789abcdef";


    if (argc != 3) {
        err_quit("USAGE: %s count size[k]", argv[0]);
    }
    count = strtoul(argv[1], NULL, 10);
    size = strtoul(argv[2], NULL, 10);

    if (size > 512) {
	size = 512;
    }

    mem_ptr = (char *)malloc(size * 1024);
    if (mem_ptr == NULL) {
        err_sys("malloc error");
    }
    ptr = mem_ptr;
    for (i = 0; i < size; i++) {
	memcpy(ptr + i*1024, content, 1024);
    }

    for (i = 0; i < 2048; i++) {
	char dir_l1[4096] = {0};

	snprintf(dir_l1, sizeof(dir_l1), "dir_level1_%ld", i);
	if (mkdir(dir_l1, 0755) < 0) {
		err_sys("mkdir(%s, 0755) error", dir_l1);
	}
	if (chdir(dir_l1) < 0) {
		err_sys("chdir(%s) error", dir_l1);
	}
	for (j = 0; j < 4096; j++) {
		char dir_l2[4096] = {0};

		snprintf(dir_l2, sizeof(dir_l2), "dir_level2_%ld", j);
		if (mkdir(dir_l2, 0755) < 0) {
			err_sys("mkdir(%s, 0755) error", dir_l2);
		}
		if (chdir(dir_l2) < 0) {
			err_sys("chdir(%s) error", dir_l2);
		}
		for (k = 0; k < count; k++) {
			char file[4096] = {0};
			int x, fd, n;
			snprintf(file, sizeof(file), "file_%ld", k);
			
			if ((fd = open(file, O_RDWR|O_CREAT, 0644)) < 0) {
				err_sys("open(%s) error", file);
			}
			for (x = 0; x < size; x++) {
				if ((n = write(fd, content, sizeof(content))) != sizeof(content)) {
					err_sys("write() error");
				}
			}
			close(fd);
		}
		if (chdir("..") < 0) {
			err_sys("chdir(..) error");
		}
	}
	if (chdir("..") < 0) {
		err_sys("chdir(..) error");
	}

        switch (i % 4) {
            case 0: printf(" -\r"); fflush(NULL); break;
            case 1: printf(" \\\r"); fflush(NULL); break;
            case 2: printf(" |\r"); fflush(NULL); break;
            case 3: printf(" /\r"); fflush(NULL); break;
        }
    }

    return 0;
}
