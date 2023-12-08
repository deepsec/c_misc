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
     err_quit("USAGE: %s [-D] [-n file_num] [-p partition_number] [-a add_pthread_num] [-d del_pthread_num] directory", cmd);
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


long gen_random_size(long min, long max)
{
	long size;

	size = random() % max;
	if (size < min) {
		size = min;
	}
	return (size);
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


void dump_info(struct partitions_buf_info *pbi)
{
	if (pbi == NULL) return;
	printf("***************************************************\n");
	printf("pbi->tindex: %ld\n", pbi->tindex);
	printf("pbi->buf: %p\n", pbi->buf);
	printf("pbi->buf_len: %ld\n", pbi->buf_len);
	printf("pbi->partition_low: %ld\n", pbi->partition_low);
	printf("pbi->partition_high: %ld\n", pbi->partition_high);
	printf("pbi->file_count: %ld\n", pbi->file_count);
	printf("***************************************************\n");
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


void *do_create_dirs_only(void *arg)
{
	struct partitions_buf_info *pbip = (struct partitions_buf_info *) arg;
	char dst_dir[PATH_MAX] = {0};
	int dir_level_1_low, dir_level_1_high;
	int i, j;
	
	// detach, so needn't pthread_join
	//pthread_detach(pthread_self());

	dir_level_1_low = pbip->partition_low;
	dir_level_1_high = pbip->partition_high;
	
	for (i = dir_level_1_low; i < dir_level_1_high; i++) {
		for (j = 0; j < 4096; j++) {
			if (j < 16) {
				snprintf(dst_dir, sizeof(dst_dir), "%s/%d/00%x", OBJECTS_DIR, i, j);
			} else if (j < 256) {
				snprintf(dst_dir, sizeof(dst_dir), "%s/%d/0%x", OBJECTS_DIR, i, j);
			} else {
				snprintf(dst_dir, sizeof(dst_dir), "%s/%d/%x", OBJECTS_DIR, i, j);
			}
			mkalldir(dst_dir, 0755);
		}
	}
	return 0;
}


void *do_create_many_files(void *arg)
{
	struct partitions_buf_info *pbip = (struct partitions_buf_info *) arg;
	struct file_info finfo = {0};	
	char file_name[NAME_MAX] = {0};
	char dst_dir[PATH_MAX] = {0};
	struct timeval	tv = {0};
	int dir_level_1_low, dir_level_1_high;
	int i, j, k;
	char md5hash_name[33] = {0}, suffix_name[4] = {0};

	//dump_info(pbip);
	dir_level_1_low = pbip->partition_low;
	dir_level_1_high = pbip->partition_high;
	
	for (i = dir_level_1_low; i < dir_level_1_high; i++) {
		for (j = 0; j < 4096; j++) {
			for (k = 0; k < pbip->file_count; k++) {
				if (gettimeofday(&tv, NULL) < 0) {
					err_sys("gettimeofday() error");
				}	
				snprintf(file_name, sizeof(file_name), "%ld.%ld.%ld.data", pbip->tindex, tv.tv_sec, tv.tv_usec);
				md5(file_name, strlen(file_name), md5hash_name);
				strncpy(suffix_name, &(md5hash_name[29]), 3);
				snprintf(dst_dir, sizeof(dst_dir), "%s/%d/%s/%s", OBJECTS_DIR, i, suffix_name, md5hash_name);
				finfo.tmp_dir = TMPFILE_DIR;
				finfo.dst_dir = dst_dir;
				finfo.file_name = file_name;
				finfo.file_size = gen_random_size(10*1024, 400*1024);
				finfo.buf = pbip->buf;
				finfo.buf_len = pbip->buf_len;
				finfo.dir_mode = 0755;
				finfo.tindex = pbip->tindex;

				create_one_file(&finfo);
			}
		}
	}
	return 0;
}


void random_remove_files(const char *dir, long count, int depth)
{
	DIR *dirp;
	struct dirent *dp;
	struct stat stbuf;
	char tmp[PATH_MAX] = {0};
		
	if (dir == NULL || count <= 0) return;
	if ((dirp = opendir(dir)) == NULL) {
		//err_msg("opendir(%s) error", dir);
		return;
	}
	while (((dp = readdir(dirp)) != NULL) && count > 0) {
		if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0) {
			continue;
		}
		snprintf(tmp, sizeof(tmp), "%s/%s", dir, dp->d_name);
		if (lstat(tmp, &stbuf) < 0) {
			err_msg("lstat(%s) error", tmp);
			continue;
		}
		if (S_ISDIR(stbuf.st_mode)) {
			random_remove_files(tmp, count, depth+1);
			if (depth == 1) {
				rmdir(tmp);
			}
			continue;
		}	
		if (S_ISREG(stbuf.st_mode)) {
			if (random() % 2 == 0) {
				if (unlink(tmp) < 0) {
					//err_msg("unlink(%s) error", tmp);
					continue;
				}
				else {
					count--;
				}
			}
		}
	}
	closedir(dirp);
	return;
}

void *do_del_files(void *arg)
{
	struct partitions_buf_info *pbip = (struct partitions_buf_info *) arg;
	char dst_dir[PATH_MAX] = {0};
	int dir_level_1_low, dir_level_1_high;
	int i, j;
	
	// detach, so needn't pthread_join
	//pthread_detach(pthread_self());

	dir_level_1_low = pbip->partition_low;
	dir_level_1_high = pbip->partition_high;
	
	for (i = dir_level_1_low; i < dir_level_1_high; i++) {
		for (j = 0; j < 4096; j++) {
			snprintf(dst_dir, sizeof(dst_dir), "%s/%d", OBJECTS_DIR, i);
			random_remove_files(dst_dir, pbip->file_count/2, 0);
		}
	}
	return 0;
}



int main(int argc, char *argv[])
{
	long partition_num, file_num, add_pthread_num, del_pthread_num, a_step, d_step;
	long i;	
	int	opt, dir_only = 0;
	pthread_t	add_tid[MAX_PTHREAD_NUM] = {0}, del_tid[MAX_PTHREAD_NUM] = {0};
	struct partitions_buf_info  pbi_array[MAX_PTHREAD_NUM], *pbi;
	char *databuf_4k = NULL;
	
	file_num = DEFAULT_FILE_NUM;
	partition_num = DEFAULT_PARTITION_NUM;
	add_pthread_num = DEFAULT_ADD_PTHREAD_NUM;
	del_pthread_num = DEFAULT_DEL_PTHREAD_NUM;
	while ((opt = getopt(argc, argv, "Dn:p:a:d:")) != -1) {
		switch (opt) {
		case 'n':
			file_num = strtoul(optarg, NULL, 10);
			break;
		case 'p':
			partition_num = strtoul(optarg, NULL, 10);
			break;
		case 'a':
			add_pthread_num = strtoul(optarg, NULL, 10);
			break;
		case 'd':
			del_pthread_num = strtoul(optarg, NULL, 10);
			break;
		case 'D':
			dir_only = 1;
			break;
		default:
			USAGE(argv[0]);
		}
	}
	//err_quit("argc[%d], %s, optind[%d]", argc, argv[optind], optind);
	if (argc != (optind+1)) {
		USAGE(argv[0]);
	}
	mkalldir(argv[optind], 0755); chdir(argv[optind]);
	if (add_pthread_num >  MAX_PTHREAD_NUM) {
		add_pthread_num = MAX_PTHREAD_NUM;
	}

	if (partition_num < add_pthread_num) {
		partition_num = add_pthread_num;
	}
	
	printf("file_num: %ld, partition_num: %ld, add_pthread_num: %ld, del_pthread_num: %ld, dir_only: %d\n", 
				file_num, partition_num, add_pthread_num, del_pthread_num, dir_only);	
	//exit(0);
	pbi = pbi_array;
	databuf_4k = gen_4k_buffer();
	if (add_pthread_num > 0) {
		a_step = partition_num / add_pthread_num;
	}
	for (i = 0; i < add_pthread_num; i++, pbi++) {		
		pbi->tindex = i;
		pbi->buf = databuf_4k;
		pbi->buf_len = 4096;
		pbi->partition_low = i * a_step;
		pbi->partition_high = pbi->partition_low  + a_step;
		pbi->file_count = file_num;
		if (dir_only == 1) {
			if (pthread_create(&add_tid[i], NULL, do_create_dirs_only, pbi) != 0) {
				perr_exit(errno, "pthread_create() error");
			}
		}
		else {
			if (pthread_create(&add_tid[i], NULL, do_create_many_files, pbi) != 0) {
				perr_exit(errno, "pthread_create() error");
			}
		}
	}

	if (del_pthread_num > 0) {
		d_step = partition_num / del_pthread_num;
	}	
	for (i = 0; i < del_pthread_num; i++) {		
		pbi->tindex = i;
		pbi->buf = NULL;
		pbi->buf_len = 0;
		pbi->partition_low = i * d_step;
		pbi->partition_high = pbi->partition_low  + d_step;
		pbi->file_count = file_num;
		if (pthread_create(&del_tid[i], NULL, do_del_files, pbi) != 0) {
			perr_exit(errno, "pthread_create() error");
		}
	}

	for (i = 0; i < add_pthread_num; i++) {		
		pthread_join(add_tid[i], NULL);
	}
	for (i = 0; i < del_pthread_num; i++) {		
		pthread_join(del_tid[i], NULL);
	}

	if(databuf_4k) {
		free(databuf_4k);
	}

	return 0;
}
