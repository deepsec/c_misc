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
#include <sys/xattr.h>
#include "ds_err.h"
#include "ds_common.h"
#include "ds_syscall.h"
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
     err_msg("USAGE: %s [-D] [-v] [-n file_num] [-p partition_num] -s [1:10:1 (file_size)]  [-a add_pthread_num] [-d del_pthread_num] [-i del_interval] [-t tmp_dir_num] directory", cmd);
	 err_msg("       -D                       only create partitions and suffix directorys, no files create");
	 err_msg("       -v                       support version when delete object, [default: no support]");
	 err_msg("       -n file_num              files in a partition = (partiton_number * 4096 * file_num), [default: %d]", DEFAULT_FILE_NUM);
	 err_msg("       -p partition_num         partitions (TOP DIR in 'objects'), [default: %d]", DEFAULT_PARTITION_NUM);
	 err_msg("       -s min:max:step          file_size, KB, [default: %d:%d:%d] ", DEFAULT_FILE_SIZE_MIN, DEFAULT_FILE_SIZE_MAX, DEFAULT_FILE_SIZE_STEP);
	 err_msg("       -a add_pthread_num       pthreads for create file [default: %d]", DEFAULT_ADD_PTHREAD_NUM);
	 err_msg("       -d del_pthread_num       pthreads for delete file [default: %d]  ", DEFAULT_DEL_PTHREAD_NUM);
	 err_msg("       -i interval              delete interval, [default: %d]", DEFAULT_DEL_INTERVAL);
	 err_msg("       -t tmp_dir_num           tmp_dir_num, [default: %d]", DEFAULT_TMPDIR_NUM);
	 err_quit("       directory                target directory name");
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

long decide_file_size(long tsum, long tindex, long file_size_min, long file_size_max, long file_size_step)
{
	long size = 0;

	if (file_size_min == file_size_max) {
		size = file_size_min;
		return size;
	}
	if (file_size_step == 0) {
		size = gen_random_size(file_size_min, file_size_max);
		return size;
	}
	size = (file_size_min + tindex * file_size_step) % file_size_max;
	return size;
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
	char xattr[4096] = {0};
	unsigned long total_size = fi->file_size, xattr_len;
	int i, n, blocks, left;
	mode_t mode = fi->dir_mode;
	DIR	*dirp;

	if ((dirp = opendir(fi->tmp_dir)) == NULL) {
		mkalldir(fi->tmp_dir, mode);
	} else {
		closedir(dirp);
	}
/*
	if ((dirp = opendir(fi->dst_dir)) == NULL) {
		mkalldir(fi->dst_dir, mode);
	} else {
		closedir(dirp);
	}
*/
	// because the dir always is not exists, so don't opendir and directly mkalldir
	mkalldir(fi->dst_dir, mode);
	snprintf(tmp_file, sizeof(tmp_file), "%s/%s", fi->tmp_dir, fi->file_name);
	snprintf(dst_file, sizeof(dst_file), "%s/%s", fi->dst_dir, fi->file_name);
	
	fd = sleep_open(tmp_file, O_RDWR | O_CREAT, 0600);
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
	// set xattr
	xattr_len = snprintf(xattr, sizeof(xattr), "user.oss.meta:content-length=%ld", total_size);
	xattr_len += snprintf(xattr + xattr_len, sizeof(xattr) - xattr_len, ",user.oss.meta:etag=%s", fi->file_name);
	if (setxattr(tmp_file, "user.oss.meta", xattr, xattr_len, 0) < 0) {
		err_sys("setxattr(%s) error", tmp_file);
	}

	fsync(fd);
	close(fd);
	sleep_rename(tmp_file, dst_file);

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
	char tmpfile_dir[NAME_MAX] = {0};
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
				snprintf(file_name, sizeof(file_name), "%ld.%ld.%ld.%ld.data", pbip->tindex, random(), tv.tv_sec, tv.tv_usec);
				md5(file_name, strlen(file_name), md5hash_name);
				strncpy(suffix_name, &(md5hash_name[29]), 3);
				snprintf(dst_dir, sizeof(dst_dir), "%s/%d/%s/%s", OBJECTS_DIR, i, suffix_name, md5hash_name);
				if (pbip->tmp_dir_num > 1) {
					snprintf(tmpfile_dir, sizeof(tmpfile_dir), "%s%ld", TMPFILE_DIR,  j % pbip->tmp_dir_num);
				} else {
					snprintf(tmpfile_dir, sizeof(tmpfile_dir), "%s", TMPFILE_DIR);
				};
				finfo.tmp_dir = tmpfile_dir;
				finfo.dst_dir = dst_dir;
				finfo.file_name = file_name;
				finfo.file_size = decide_file_size(pbip->tsum, pbip->tindex, pbip->file_size_min, pbip->file_size_max, pbip->file_size_step);
				finfo.buf = pbip->buf;
				finfo.buf_len = pbip->buf_len;
				finfo.dir_mode = 0755;
				finfo.tindex = pbip->tindex;

				create_one_file(&finfo);
				pbip->file_total_add++;
				pbip->file_total_add_bytes += finfo.file_size;
			}
		}
	}
	return 0;
}


void random_remove_files(struct partitions_buf_info *pbip, const char *dir, long *file_total)
{
	DIR *dirp;
	struct dirent *dp;
	struct stat stbuf;
	struct timeval	tv = {0};
	char tmp[PATH_MAX] = {0}, version_file[PATH_MAX] = {0};
	char ts_name[NAME_MAX] = {0}, ts_tmp[PATH_MAX] = {0}, ts_dst_name[PATH_MAX] = {0};
	int ts_fd;
		
	if (dir == NULL) return;
	if ((dirp = opendir(dir)) == NULL) {
		//err_msg("opendir(%s) error", dir);
		return;
	}
	while ((dp = readdir(dirp)) != NULL) {
		if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0) {
			continue;
		}
		snprintf(tmp, sizeof(tmp), "%s/%s", dir, dp->d_name);
		if (lstat(tmp, &stbuf) < 0) {
			err_msg("lstat(%s) error", tmp);
			continue;
		}
		if (S_ISDIR(stbuf.st_mode)) {
			random_remove_files(pbip, tmp, file_total);
			/*
			if (depth == 1) {
				rmdir(tmp);
			}
			*/
			continue;
		}	
		if (S_ISREG(stbuf.st_mode)) {
			(*file_total)++;
			if (*file_total % 2 == 0) {
				if (gettimeofday(&tv, NULL) < 0) {
					err_sys("gettimeofday() error");
				}	
				snprintf(ts_name, sizeof(ts_name), "%ld.%ld.%ld.%ld.ts", pbip->tindex, random(), tv.tv_sec, tv.tv_usec);
				snprintf(ts_tmp, sizeof(ts_tmp), "%s/%s", TMPFILE_DIR, ts_name);
				snprintf(ts_dst_name, sizeof(ts_dst_name), "%s/%s", dir, ts_name);
				ts_fd = sleep_open(ts_tmp, O_RDWR | O_CREAT, 0600);
				fsync(ts_fd);
				close(ts_fd);
				if (pbip->have_version) {
					snprintf(version_file, sizeof(version_file), "%s/version.%s_%ld.%ld.%ld.%ld.data", dir, dp->d_name, pbip->tindex, random(), tv.tv_sec, tv.tv_usec);
					sleep_rename(tmp, version_file);
				} else {
					if (unlink(tmp) < 0) {
						err_ret("unlink(%s) error", tmp);
					}					
				}
				sleep_rename(ts_tmp, ts_dst_name);
				pbip->file_total_del++;
				pbip->file_total_del_bytes += stbuf.st_size;		
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
	int i;
	long file_total = 0;
	
	// detach, so needn't pthread_join
	//pthread_detach(pthread_self());
	dir_level_1_low = pbip->partition_low;
	dir_level_1_high = pbip->partition_high;
	for (i = dir_level_1_low; i < dir_level_1_high; i++) {
		snprintf(dst_dir, sizeof(dst_dir), "%s/%d", OBJECTS_DIR, i);
		random_remove_files(pbip, dst_dir, &file_total);
	}
	return 0;
}

void parse_size_format(char *format, long *size_min, long *size_max, long *size_step)
{
	char *p, *p2, v[256] = {0};
	int i, mark;

	if (format == NULL || strlen(format) >= 255 || !size_min || !size_max || !size_step)
		return;
	//err_msg("format = %s", format);
	p2 = format, p = format;
	for (i = 0, mark = 0; i <= strlen(format); i++) {
		if (p2[i] != ':' && p2[i] != '\0') {
			continue;
		}
		mark++;
		strncpy(v, p, &p2[i] - p);
		p = &p2[i+1];
		switch (mark) {
		case 1:	
			*size_min = strtoul(v, NULL, 10); 
			break;
		case 2: 
			*size_max = strtoul(v, NULL, 10); 
			break;
		case 3: 
			*size_step = strtoul(v, NULL, 10); 
			break;
		}
		memset(v, 0, sizeof(v));
	}
	if (*size_max < *size_min || *size_step > (DEFAULT_FILE_SIZE_MAX - DEFAULT_FILE_SIZE_MIN) || *size_step > (*size_max-*size_min)) {
		err_quit("size range error: min[%ld],max[%ld],step[%lu]", *size_min, *size_max, *size_step);
	}
	if (*size_min < DEFAULT_FILE_SIZE_MIN) {
		*size_min = DEFAULT_FILE_SIZE_MIN;
	}
	if (*size_max > DEFAULT_FILE_SIZE_MAX) {
		*size_max = DEFAULT_FILE_SIZE_MAX;
	}
	return;
}

void get_statistic_info(struct statistic_info *si)
{
	struct partitions_buf_info *pbi_add = si->pbi_add;
	struct partitions_buf_info *pbi_del = si->pbi_del;
	int i;

	si->add_total = 0;
	si->del_total = 0;
	si->add_total_bytes = 0;
	si->del_total_bytes = 0;
	for (i = 0; i < si->pbi_add_len; i++) {
		si->add_total += pbi_add[i].file_total_add;
		si->add_total_bytes += pbi_add[i].file_total_add_bytes;
	}
	for (i = 0; i < si->pbi_del_len; i++) {
		si->del_total += pbi_del[i].file_total_del;
		si->del_total_bytes += pbi_del[i].file_total_del_bytes;		
	}
	return;
}

void print_cmdline(int argc, char **argv)
{
	printf("*******************************************            DIRECTORY : %s           ********************************************\n", argv[argc-1]);
	return;	
}

void *do_statistic(void *arg)
{
	struct statistic_info *si = (struct statistic_info *) arg;
	long cur_add_total = 0, last_add_total = 0, add_inc = 0, add_inc_max = 0, add_inc_min = 0;
	long cur_add_total_bytes = 0, last_add_total_bytes = 0, add_inc_bytes = 0, add_inc_bytes_max = 0, add_inc_bytes_min = 0;
	long cur_del_total = 0, last_del_total = 0, del_dec = 0, del_dec_max = 0, del_dec_min = 0;
	long cur_del_total_bytes = 0, last_del_total_bytes = 0, del_dec_bytes = 0, del_dec_bytes_max = 0, del_dec_bytes_min = 0;
	long time_elapsed = 0;
	int add_inc_mark = 0, add_inc_bytes_mark = 0, del_dec_mark = 0, del_dec_bytes_mark = 0;
	
	// detach, so needn't pthread_join
	pthread_detach(pthread_self());
	print_cmdline(si->cmdline_len, si->cmdline);
	for (;;) {
		last_add_total = cur_add_total;
		last_add_total_bytes = cur_add_total_bytes;
		last_del_total = cur_del_total;
		last_del_total_bytes = cur_del_total_bytes;	
		sleep(1);
		time_elapsed++;
		get_statistic_info(si);
		cur_add_total = si->add_total;
		cur_add_total_bytes = si->add_total_bytes;
		cur_del_total = si->del_total;
		cur_del_total_bytes = si->del_total_bytes;

		add_inc = cur_add_total - last_add_total;
		// get the first normal value
		if (add_inc_mark == 0 && add_inc > 0) {
			add_inc_max = add_inc_min = add_inc;
			add_inc_mark = 1;
		}
		if (add_inc > add_inc_max && add_inc > 0) {
			add_inc_max = add_inc;
		} 
		if (add_inc < add_inc_min && add_inc > 0) {
			add_inc_min = add_inc;		
		}
		add_inc_bytes = cur_add_total_bytes - last_add_total_bytes;
		// get the first normal value
		if (add_inc_bytes_mark == 0 && add_inc_bytes > 0) {
			add_inc_bytes_max = add_inc_bytes_min = add_inc_bytes;
			add_inc_bytes_mark = 1;
		}
		if (add_inc_bytes > add_inc_bytes_max && add_inc_bytes > 0) {
			add_inc_bytes_max = add_inc_bytes;
		} 
		if (add_inc_bytes_min < add_inc_min && add_inc_bytes > 0) {
			add_inc_bytes_min = add_inc_bytes;		
		}		
		del_dec = cur_del_total - last_del_total;
		// get the first normal value
		if (del_dec_mark == 0 && del_dec > 0) {
			del_dec_max = del_dec_min = del_dec;
			del_dec_mark = 1;
		}
		if (del_dec > del_dec_max && del_dec > 0) {
			del_dec_max = del_dec;
		}
		if (del_dec < del_dec_min && del_dec > 0) {
			del_dec_min = del_dec;
		}
        del_dec_bytes = cur_del_total_bytes - last_del_total_bytes;
		// get the first normal value
		if (del_dec_bytes_mark == 0 && del_dec_bytes > 0) {
			del_dec_bytes_max = del_dec_bytes_min = del_dec_bytes;
			del_dec_bytes_mark = 1;
		}
		if (del_dec_bytes > del_dec_bytes_max && del_dec_bytes > 0) {
			del_dec_bytes_max = del_dec_bytes;
		}
		if (del_dec_bytes < del_dec_bytes_min && del_dec_bytes > 0) {
			del_dec_bytes_min = del_dec_bytes;
		}
		if (time_elapsed % 20 == 0) {
			print_cmdline(si->cmdline_len, si->cmdline);
		}
		if (si->pbi_add_len > 0) {
			err_msg("ADD:    files:    total: %ld, speed: %ld/s, max: %ld/s, min: %ld/s, elapsed: %lds, average: %ld/s",
				cur_add_total, add_inc, add_inc_max, add_inc_min, time_elapsed, cur_add_total/time_elapsed);
			if (si->print_bytes_info) {
				err_msg("        bytes:    total: %ld, speed: %ld/s, max: %ld/s, min: %ld/s, elapsed: %lds, average: %ld/s",
					cur_add_total_bytes, add_inc_bytes, add_inc_bytes_max, add_inc_bytes_min, time_elapsed, cur_add_total_bytes/time_elapsed);
			}
		}
		
		if (si->pbi_del_len > 0) {
			err_msg("DEL:    files:    total: %ld, speed: %ld/s, max: %ld/s, min: %ld/s, time_elapsed: %lds, average: %ld/s", 
					cur_del_total, del_dec, del_dec_max, del_dec_min, time_elapsed, cur_del_total/time_elapsed);
			if (si->print_bytes_info) {
				err_msg("        bytes:    total: %ld, speed: %ld/s, max: %ld/s, min: %ld/s, time_elapsed: %lds, average: %ld/s", 
					cur_del_total_bytes, del_dec_bytes, del_dec_bytes_max, del_dec_bytes_min, time_elapsed, cur_del_total_bytes/time_elapsed);		
			}
		}	
	}
	return 0;
}

int main(int argc, char *argv[])
{
	long partition_num, file_num, add_pthread_num, del_pthread_num, a_step, d_step, del_interval;
	long file_size_min, file_size_max, file_size_step, tmp_dir_num;
	long i;	
	int	opt, dir_only = 0, have_version = 0, print_bytes_info = 0;
	pthread_t	add_tid[MAX_PTHREAD_NUM] = {0}, del_tid[MAX_PTHREAD_NUM] = {0}, stat_tid;
	struct partitions_buf_info  pbi_add_array[MAX_PTHREAD_NUM] = {{0},}, pbi_del_array[MAX_PTHREAD_NUM] = {{0},}, *pbi;
	struct statistic_info si = {0};
	char *databuf_4k = NULL;
	
	file_num = DEFAULT_FILE_NUM;
	partition_num = DEFAULT_PARTITION_NUM;
	add_pthread_num = DEFAULT_ADD_PTHREAD_NUM;
	del_pthread_num = DEFAULT_DEL_PTHREAD_NUM;
	file_size_min = DEFAULT_FILE_SIZE_MIN;
	file_size_max = DEFAULT_FILE_SIZE_MAX;
	file_size_step = DEFAULT_FILE_SIZE_STEP;
	del_interval = DEFAULT_DEL_INTERVAL;
	tmp_dir_num = DEFAULT_TMPDIR_NUM;
	while ((opt = getopt(argc, argv, "Bn:p:s:a:d:Dvi:t:w:")) != -1) {
		switch (opt) {
		case 'n':
			file_num = strtoul(optarg, NULL, 10);
			break;
		case 'p':
			partition_num = strtoul(optarg, NULL, 10);
			break;
		case 's':
			parse_size_format(optarg, &file_size_min, &file_size_max, &file_size_step);
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
		case 'v':
			have_version = 1;
			break;
		case 'B':
			print_bytes_info = 1;
			break;
		case 'i':
			del_interval = strtoul(optarg, NULL, 10);
			break;
		case 't':
			tmp_dir_num = strtoul(optarg, NULL, 10);
			break;
		default:
			USAGE(argv[0]);
		}
	}
	//err_quit("argc[%d], %s, optind[%d]", argc, argv[optind], optind);
	if (argc != (optind+1)) {
		USAGE(argv[0]);
	}
	mkalldir(argv[optind], 0755); 
	if (chdir(argv[optind]) < 0) {
		err_sys("chdir error");
	}
	if (add_pthread_num >  MAX_PTHREAD_NUM) {
		add_pthread_num = MAX_PTHREAD_NUM;
	}
	if (del_pthread_num >  MAX_PTHREAD_NUM) {
		del_pthread_num = MAX_PTHREAD_NUM;
	}
	if (partition_num < add_pthread_num || partition_num < del_pthread_num) {
		err_quit("partition_num must be great than add&del pthread");
	}
	if (partition_num < 1) {
		partition_num = 1;
	}	
	err_msg("\nCommand Lines Options:");
	err_msg("\tfile_num: %ld, file_size[%ld:%ld:%ld]", file_num, file_size_min, file_size_max, file_size_step); 
	err_msg("\tpartition_num: %ld, add_pthread_num: %ld, del_pthread_num: %ld", partition_num, add_pthread_num, del_pthread_num);
	err_msg("\tdir_only: %d, version: %d, del_interval:%ld, tmp_dir_num: %ld\n", dir_only, have_version, del_interval, tmp_dir_num);	
	//exit(0);
	pbi = pbi_add_array;
	databuf_4k = gen_4k_buffer();
	if (add_pthread_num > 0) {
		a_step = partition_num / add_pthread_num;
	}
	for (i = 0; i < add_pthread_num; i++, pbi++) {	
		pbi->tsum = add_pthread_num;	
		pbi->tindex = i;
		pbi->buf = databuf_4k;
		pbi->buf_len = 4096;
		pbi->partition_low = i * a_step;
		pbi->partition_high = pbi->partition_low  + a_step;
		pbi->file_count = file_num;
		pbi->file_size_min = file_size_min * 1024;  //from KB to bytes
		pbi->file_size_max = file_size_max * 1024;
		pbi->file_size_step = file_size_step * 1024;
		pbi->tmp_dir_num = tmp_dir_num;
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

	si.pbi_add = pbi_add_array;
	si.pbi_add_len = add_pthread_num;
	si.pbi_del = pbi_del_array;
	si.pbi_del_len = del_pthread_num;
	si.cmdline = argv;
	si.cmdline_len = argc;
	si.print_bytes_info = print_bytes_info;
	if (pthread_create(&stat_tid, NULL, do_statistic, &si) != 0) {
		perr_exit(errno, "pthread_create() error");
	}

	if (del_pthread_num > 0) {
		d_step = partition_num / del_pthread_num;
	}
	pbi = pbi_del_array;
	sleep(del_interval);	
	for (i = 0; i < del_pthread_num; i++) {
		pbi->tsum = del_pthread_num;	
		pbi->tindex = i;
		pbi->buf = NULL;
		pbi->buf_len = 0;
		pbi->partition_low = i * d_step;
		pbi->partition_high = pbi->partition_low  + d_step;
		pbi->file_count = file_num;
		pbi->have_version = have_version;
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
	sleep(3);
	err_msg ("finished all work!");

	return 0;
}
