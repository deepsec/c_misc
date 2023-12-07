#ifndef __OSSIO_H__
#define __OSSIO_H__

struct file_info {
	char *tmp_dir;
	char *dst_dir;
	mode_t	dir_mode;
	char *file_name;
	unsigned long file_size;
	char *buf;
	unsigned long buf_len;
};


struct partitions_buf_info {
	long tindex;
	char *buf;
	long buf_len;
	long partition_low;
	long partition_high;
	long file_count;
};


#define TMPFILE_DIR					"tmp"
#define OBJECTS_DIR					"objects"
#define DEFAULT_PARTITION_NUM		1024
#define DEFAULT_FILE_NUM			2
#define MAX_FILE_NUM				(16 * 1024)
#define DEFAULT_ADD_PTHREAD_NUM		2
#define DEFAULT_DEL_PTHREAD_NUM		0
#define MAX_PTHREAD_NUM				8192

#endif
