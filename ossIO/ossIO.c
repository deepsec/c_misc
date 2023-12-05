#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <openssl/md5.h>
#include <pthread.h>
#include "ds_err.h"

#define TMPFILE_DIR	"tmp"
#define OBJECTS_DIR	"objects"
#define DEAULT_FILE_SIZE	8
#define DEFAULT_FILE_NUMBER	1
#define DEFAULT_PARTITION_NUMBER	1024

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
     printf("USAGE: %s -d tmpdir -s file_size -n file_number -p partition_number", cmd);
	 printf("      1. tmpdir for 'tmp' directory, default './tmp'");
	 printf("      2. file_size is in KB, so 16 means 16KB, max is 512");
	 printf("      3. file_number is the number of files per directory, default 1");
	 printf("      4. partition_number is the FIRST LEVEL directorys, default 1024");
	 exit(1);
}

void *md5(const char *str, int len, char *output)
{
    int i;
    unsigned char digest[MD5_DIGEST_LENGTH] = {0};

    MD5((unsigned char *)str, len, (unsigned char *)digest);

    for (i = 0; i < MD5_DIGEST_LENGTH; i++) {
        sprintf(output+2*i,"%02x", digest[i]);
    }
    
    return output;
}

void mkAllDir(char *dir)
{
	char tmp[NAME_MAX];
	char *p, *p2;

	// must be relative path
	if (dir == NULL || *dir == '/') return;
	for (p = dir; p && p < dir + strlen(dir);) {
		while(*p == '/') p++;
		if ((p2 = strchr(p, '/')) != NULL) {
			memset(tmp, 0, sizeof(tmp));
			strncpy(tmp, p, p2 - p);
			if (mkdir(tmp, 0755) < 0) {
				if (errno != EEXIST) {
					err_sys("mkdir(%s, 0755) error", tmp);
				}
			}
			chdir(tmp);
		} else {
			if (*p != '\0') {
				memset(tmp, 0, sizeof(tmp));
				strncpy(tmp, p, dir + strlen(dir) - p);
				if (mkdir(tmp, 0755) < 0) {
					if (errno != EEXIST) {
						err_sys("mkdir(%s, 0755) error", tmp);
					}
				}
				chdir(tmp);
			}
		}
		p = p2;
	}
	return;
}

struct file_info {
	char *tmp_dir;
	char *dst_dir;
	char *buf;
	unsigned long buf_len;
};

void *create_file(void *arg)
{
	struct file_info *fi = (struct file_info *) arg;
	int fd;
	struct timeval	*tv;
	char dst_file[PATH_MAX] = {0};
	char tmp_file[PATH_MAX] = {0};

	if (gettimeofday(&tv, NULL) < 0) {
		err_sys("gettimeofday() error");
	}
	snprintf(tmp_file, sizeof(tmp_file), "%s/%ld.%ld.data", fi->tmp_dir, tv->tv_sec, tv->tv_usec);
	snprintf(dst_file, sizeof(dst_file), "%s/%ld.%ld.data", fi->dst_dir, tv->tv_sec, tv->tv_usec);
	if ((fd = open(tmp_file, O_RDWR | O_CREAT, 0600)) < 0) {
		err_sys("open(%s) error", tmp_file);
	}
	if ((n = write(fd, fi->buf, fi->buf_len)) != fi->buf_len) {
		err_sys("write() error");
	}
	fsync(fd);
	close(fd);
	rename(tmp_file, dst_file);
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
	while ((opt = getopt(argc, argv, "d:s:n:p:")) != -1) {
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
		case 'p':
			partitions_number = strtoul(optarg, NULL, 10);
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
	printf("tmp_dir: %s,  file_size: %ldk, file_number: %ld, partitions_number: %ld\n", tmp_dir, file_size, file_number, partitions_number);	

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
