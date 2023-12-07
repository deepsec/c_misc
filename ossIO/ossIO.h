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



#endif
