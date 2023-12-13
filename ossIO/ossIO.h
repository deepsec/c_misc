#ifndef __OSSIO_H__
#define __OSSIO_H__

struct file_info {
	long tindex;
	char *tmp_dir;
	char *dst_dir;
	mode_t	dir_mode;
	char *file_name;
	unsigned long file_size;
	char *buf;
	unsigned long buf_len;
};


struct partitions_buf_info {
	long tsum;
	long tindex;
	char *buf;
	long buf_len;
	long partition_low;
	long partition_high;
	long file_count;
	long file_size_min;
	long file_size_max;
	long file_size_step;
	long file_total_add;
	long file_total_add_bytes;
	long file_total_del;
	long file_total_del_bytes;
	int  have_version;
	long tmp_dir_num;
};

struct statistic_info {
	struct partitions_buf_info *pbi_add, *pbi_del;
	int	pbi_add_len, pbi_del_len;
	long add_total;
	long add_total_bytes;
	long del_total;
	long del_total_bytes;
	char **cmdline;
	int cmdline_len;
	int print_bytes_info;
};

#define TMPFILE_DIR					"tmp"
#define OBJECTS_DIR					"objects"
#define DEFAULT_PARTITION_NUM		128
#define DEFAULT_FILE_NUM			2
#define MAX_FILE_NUM				1024
#define DEFAULT_FILE_SIZE_MAX		512
#define DEFAULT_FILE_SIZE_MIN		8
#define DEFAULT_FILE_SIZE_STEP		20
#define DEFAULT_ADD_PTHREAD_NUM		2
#define DEFAULT_DEL_PTHREAD_NUM		0
#define MAX_PTHREAD_NUM				128
#define DEFAULT_DEL_INTERVAL		60
#define DEFAULT_TMPDIR_NUM			1

#endif
