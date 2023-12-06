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
	char *buf;
	int buf_len;
	int partition_low;
	int partition_high;
	int file_count;
};



#endif
