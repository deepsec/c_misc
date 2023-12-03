#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "ds_err.h"

void show_status(int num)
{
	if (num <= 0) {
		return;
	}

	switch (num % 4) {
		case 0:
			printf(" -\r");
			fflush(NULL);
			break;
		case 1:
			printf(" \\\r");
			fflush(NULL);
			break;
		case 2:
			printf(" |\r");
			fflush(NULL);
			break;
		case 3:
			printf(" /\r");
			fflush(NULL);
			break;
	}

}

int main(int argc, char *argv[])
{
	unsigned long file_number, file_size;
	unsigned long i, j, k;
	char *buf, *ptr;
	unsigned long buf_len;

	char content[16] = "123456789abcdef";
	unsigned int content_len = sizeof(content), bs = 0;

	if (argc != 3) {
		err_quit("USAGE: %s file_number[sum: 1024*4096*file_number] file_size[k, max=512]", argv[0]);
	}
	file_number = strtoul(argv[1], NULL, 10);
	file_size = strtoul(argv[2], NULL, 10);
	if (file_size > 512) {
		file_size = 512;
	}

	buf_len = file_size * 1024;
	buf = (char *) malloc(buf_len);
	if (buf == NULL) {
		err_sys("malloc error");
	}
	ptr = buf;
	bs = buf_len / content_len;
	for (i = 0; i < bs; i++) {
		memcpy(ptr + i * content_len, content, content_len);
	}

	for (i = 0; i < 1024; i++) {
		char dir_level1[NAME_MAX] = { 0 };
		snprintf(dir_level1, sizeof(dir_level1), "dir_level1_%ld", i);
		if (mkdir(dir_level1, 0755) < 0) {
			err_sys("mkdir(%s, 0755) error", dir_level1);
		}
		if (chdir(dir_level1) < 0) {
			err_sys("chdir(%s) error", dir_level1);
		}
		for (j = 0; j < 4096; j++) {
			char dir_level2[NAME_MAX] = { 0 };
			snprintf(dir_level2, sizeof(dir_level2), "dir_level2_%ld", j);
			if (mkdir(dir_level2, 0755) < 0) {
				err_sys("mkdir(%s, 0755) error", dir_level2);
			}
			if (chdir(dir_level2) < 0) {
				err_sys("chdir(%s) error", dir_level2);
			}
			for (k = 0; k < file_number; k++) {
				char file[NAME_MAX] = { 0 };
				int fd, n;
				snprintf(file, sizeof(file), "file_%ld", k);

				if ((fd = open(file, O_RDWR | O_CREAT, 0644)) < 0) {
					err_sys("open(%s) error", file);
				}
				if ((n = write(fd, buf, buf_len)) != buf_len) {
					err_sys("write() error");
				}
				close(fd);
			}
			show_status(j + 1);
			if (chdir("..") < 0) {
				err_sys("chdir(..) error");
			}
		}
		if (chdir("..") < 0) {
			err_sys("chdir(..) error");
		}
	}

	return 0;
}
