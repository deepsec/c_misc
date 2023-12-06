#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <errno.h>
#include <openssl/md5.h>
#include <pthread.h>
#include <sys/time.h>
#include "ds_err.h"
#include "ds_common.h"
#include "ossIO.h"

#define TMPFILE_DIR	"tmp"
#define OBJECTS_DIR	"objects"
#define DEFAULT_PARTITION_NUMBER	1024
#define DEFAULT_FILE_NUMBER		1
#define MAX_FILE_NUMBER			(16 * 1024)
#define DEFAULT_PTHREAD_NUMBER	2
#define MAX_PHTREAD_NUMBER		1024

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
     printf("USAGE: %s [-n file_number] [-p partition_number] [-t pthread_number] directory \n", cmd);
	 printf("      1. file_number is the number of files per directory,[%d~%d], default:[%d]\n", DEFAULT_FILE_NUMBER, MAX_FILE_NUMBER, DEFAULT_FILE_NUMBER);
	 printf("      2. partition_number is the FIRST LEVEL directorys, default %d\n", DEFAULT_PARTITION_NUMBER);
	 printf("      3. pthread_number is the pthread number, [%d~%d], default:[%d]\n", DEFAULT_PTHREAD_NUMBER, MAX_PHTREAD_NUMBER, DEFAULT_PTHREAD_NUMBER);
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

int gen_random_size(int min, int max)
{
	int size;

	size = random() % max;
	if (size < min) {
		size = min;
	}
	return (size * 1024);
}

char *gen_4k_buffer()
{
	char *common_buf, *ptr;
	int common_buf_len = 4096, content_len, blocks;
	char content[17] = "0123456789abcdef";
	int i;

	content_len = strlen(content);
	common_buf = (char *) malloc(common_buf_len);
	if (common_buf == NULL) {
		err_sys("malloc error");
	}
	ptr = common_buf;
	blocks = common_buf_len / content_len;
	for (i = 0; i < blocks; i++) {
		memcpy(ptr + i * content_len, content, content_len);
	}
	return common_buf;
}


int create_one_file(struct file_info *finfo)
{
	struct file_info *fi = finfo;
	int fd;
	char dst_file[PATH_MAX] = {0};
	char tmp_file[PATH_MAX] = {0};
	unsigned long total_size = fi->file_size;
	int i, n, blocks, left;
	mode_t mode = fi->dir_mode;
	DIR	*dirp;

	if ((dirp = opendir(fi->tmp_dir)) == NULL) {
		mkalldir(fi->tmp_dir, mode);
	} else {
		closedir(dirp);
	}
	if ((dirp = opendir(fi->dst_dir)) == NULL) {
		mkalldir(fi->dst_dir, mode);
	} else {
		closedir(dirp);
	}
	snprintf(tmp_file, sizeof(tmp_file), "%s/%s", fi->tmp_dir, fi->file_name);
	snprintf(dst_file, sizeof(dst_file), "%s/%s", fi->dst_dir, fi->file_name);
	
	if ((fd = open(tmp_file, O_RDWR | O_CREAT, 0600)) < 0) {
		err_sys("open(%s) error", tmp_file);
	}
	blocks = total_size / fi->buf_len;
	left = total_size % fi->buf_len;
	// whole blocks for write, every block is fi->buf_len
	for (i = 0; i < blocks; i++) {
		if ((n = writen(fd, fi->buf, fi->buf_len)) < 0) {
			err_sys("writen() error, return value[%d]", n);
		}
	}
	// last block for write, size is left
	if ((n = writen(fd, fi->buf, left)) < 0) {
		err_sys("writen() error, return value[%d]", n);
	}
	fsync(fd);
	close(fd);
	if (rename(tmp_file, dst_file) < 0) {
		err_sys("rename(%s, %s) error", tmp_file, dst_file);
	};

	return 0;
}

void *create_many_files(void *arg)
{
	struct partitions_buf_info *pbip = (struct partitions_buf_info *) arg;
	struct file_info finfo = {0};	
	char file_name[NAME_MAX] = {0};
	char dst_dir[PATH_MAX] = {0};
	struct timeval	tv = {0};
	int dir_level_1_low, dir_level_1_high;
	int i, j, k;
	char md5hash_name[33] = {0}, suffix_name[4] = {0};

	dir_level_1_low = pbip->partition_low;
	dir_level_1_high = pbip->partition_high;
	
	for (i = dir_level_1_low; i < dir_level_1_high; i++) {
		for (j = 0; j < 4096; j++) {
			for (k = 0; k < pbip->file_count; k++) {
				if (gettimeofday(&tv, NULL) < 0) {
					err_sys("gettimeofday() error");
				}	
				snprintf(file_name, sizeof(file_name), "%ld.%ld.data", tv.tv_sec, tv.tv_usec);
				md5(file_name, strlen(file_name), md5hash_name);
				strncpy(suffix_name, &(md5hash_name[29]), 3);
				snprintf(dst_dir, sizeof(dst_dir), "%s/%d/%s/%s", OBJECTS_DIR, i, suffix_name, md5hash_name);
				finfo.tmp_dir = TMPFILE_DIR;
				finfo.dst_dir = dst_dir;
				finfo.file_name = file_name;
				finfo.file_size = gen_random_size(10, 128);
				finfo.buf = pbip->buf;
				finfo.buf_len = pbip->buf_len;
				finfo.dir_mode = 0755;

				create_one_file(&finfo);
			}
		}
	}
	return 0;
}


int main(int argc, char *argv[])
{
	unsigned long partitions_number, file_number, pthread_number, p_step;
	unsigned long i;	
	int	opt;
	pthread_t tid[MAX_PHTREAD_NUMBER] = {0};
	char *databuf_4k = NULL;
	
	file_number = DEFAULT_FILE_NUMBER;
	partitions_number = DEFAULT_PARTITION_NUMBER;
	pthread_number = DEFAULT_PTHREAD_NUMBER;
	while ((opt = getopt(argc, argv, "n:p:t:")) != -1) {
		switch (opt) {
		case 'n':
			file_number = strtoul(optarg, NULL, 10);
			break;
		case 'p':
			partitions_number = strtoul(optarg, NULL, 10);
			break;
		case 't':
			pthread_number = strtoul(optarg, NULL, 10);
			break;
		default:
			USAGE(argv[0]);
		}
	}
	//err_quit("argc: %d, %s", argc, argv[optind]);
	if (argc != (optind+1)) {
		USAGE(argv[0]);
	}
	mkalldir(argv[optind], 0755); chdir(argv[optind]);
	if (pthread_number >  MAX_PHTREAD_NUMBER) {
		pthread_number = MAX_PHTREAD_NUMBER;
	}

	if (partitions_number < pthread_number) {
		partitions_number = pthread_number;
	}
	p_step = partitions_number / pthread_number;	
	printf("file_number: %ld, partitions_number: %ld, pthread: %ld\n", file_number, partitions_number, pthread_number);	
	//exit(0);
	databuf_4k = gen_4k_buffer();	
	for (i = 0; i < pthread_number; i++) {
		struct partitions_buf_info pbi = {0};
		pbi.buf = databuf_4k;
		pbi.buf_len = 4096;
		pbi.partition_low = i * p_step;
		pbi.partition_high = pbi.partition_low  + p_step;
		pbi.file_count = file_number;
		if (pthread_create(&tid[i], NULL, create_many_files, &pbi) != 0) {
			perr_exit(errno, "pthread_create() error");
		};

	}
	for (i = 0; i < pthread_number; i++) {		
		pthread_join(tid[i], NULL);
	}

	if(databuf_4k) {
		free(databuf_4k);
	}

	return 0;
}
