#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "ds_err.h"

#define TMP_DIR		"tmp"
#define OBJECTS_DIR	"objects"
#define DEAULT_FILE_SIZE	8
#define DEFAULT_FILE_NUMBER	1

void show_status(int num)
{
	if (num <= 0) {
		return;
	}
	switch (num % 4) {
		case 0: printf(" -\r");	fflush(NULL); break;
		case 1: printf(" \\\r"); fflush(NULL); break;
		case 2: printf(" |\r"); fflush(NULL); break;
		case 3: printf(" /\r"); fflush(NULL); break;
		default: break;
	}
	return;
}

void USAGE(const char *cmd)
{
     printf("USAGE: %s -d tmpdir -s file_size -n file_number", cmd);
	 printf("      1. tmpdir for 'tmp' directory, default './tmp'");
	 printf("      2. file_size is in KB, so 16 means 16KB, max is 512");
	 exit(1);
}

struct file_info {
	char *tmpdir;
	char *dst_dir;
	char *buf;
	unsigned long buf_len;
	unsigned long file_number;
};

void *create_file_per_dir(void *arg)
{
	struct file_info *fp = (struct file_info *) arg;
	int fd, i;

	for (i = 0; i < fp->file_number; i++) {
		char file[NAME_MAX] = { 0 };
		snprintf(file, sizeof(file), "file_%ld", i);

		if ((fd = open(file, O_RDWR | O_CREAT, 0644)) < 0) {
			err_sys("open(%s) error", file);
		}
		if ((n = write(fd, fp->buf, fp->buf_len)) != fp->buf_len) {
			err_sys("write() error");
		}
		close(fd);
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
	int	opt;
	char *tmp_dir = TMP_DIR;

	file_number = DEFAULT_FILE_NUMBER;
	file_size = DEFAULT_FILE_NUMBER;
	while ((opt = getopt(argc, argv, "d:s:n:")) != -1) {
		switch (opt) {
		case 'd':
			tmp_dir = optarg; 
			break;
		case 's':
			file_size = strtoul(optarg, NULL, 10);
			break;
		case 'n':
			file_number = strtoul(optarg, NULL, 10);
			break;
		default:
			USAGE(argv[0]);
		}
	}
	if (argc != 1) {
		USAGE(argv[0]);
	}
	//exit(0);
	if (file_size > 512) {
		file_size = 512;
	}
	printf("tmp_dir: %s,  file_size: %ldk, file_number: %ld\n", tmp_dir, file_size, file_number);	

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
