#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include "ds_common.h"
#include "ds_err.h"
#include "ds_syscall.h"
#include <time.h>

void mkalldir(char *dir, mode_t mode)
{
	char *p;
	char all_dir[PATH_MAX] = {0};
	int i;
	int retry_times = 0;

	if (dir == NULL) return;

	for (i = 0, p = dir; i <= strlen(dir); i++) {
		if (i == 0 && *p == '/') {	/* skip root directory */
			continue;
		}
		if (p[i] != '/' && p[i] != '\0') {
			continue;
		}
		strncpy(all_dir, dir, i);		
RETRY_MKDIR:
		if (mkdir(all_dir, mode) < 0) {
			if (errno == ENOSPC || errno == EBUSY || errno == EAGAIN || errno == EINTR) {
					syscall_sleep(SYSCALL_SLEEP_MILLISECONDS);
					if (retry_times++ < SYSCALL_CALL_MAX_RETRY_TIMES) {
						goto RETRY_MKDIR;
					}
			}
			if (errno != EEXIST) {	
				err_msg("mkdir() syscall reached max retry: %d", SYSCALL_CALL_MAX_RETRY_TIMES);				
				err_sys("mkdir(%s, %o) error", all_dir, mode);
			}
		}
	}
	return;
}

int sleep_open(const char *pathname, int flags, mode_t mode)
{
	int fd;
	int retry_times = 0;

RETRY_OPEN:
	if ((fd = open(pathname, flags, mode)) < 0) {
		if (errno == ENOSPC || errno == EBUSY || errno == EAGAIN || errno == EINTR) {
			syscall_sleep(SYSCALL_SLEEP_MILLISECONDS);
			if (retry_times++ < SYSCALL_CALL_MAX_RETRY_TIMES) {
				goto RETRY_OPEN;
			}
		}
		err_msg("open() syscall reached max retry: %d", SYSCALL_CALL_MAX_RETRY_TIMES);
		err_sys("open(%s, %o, %o) error", pathname, flags, mode);
	}
	return fd;
}	

int sleep_rename(const char *oldpath, const char *newpath)
{
	int ret;
	int retry_times = 0;

RETRY_RENAME:
	if ((ret = rename(oldpath, newpath)) < 0) {
		if (errno == ENOSPC || errno == EBUSY || errno == EAGAIN || errno == EINTR) {
			syscall_sleep(SYSCALL_SLEEP_MILLISECONDS);
			if (retry_times++ < SYSCALL_CALL_MAX_RETRY_TIMES) {
				goto RETRY_RENAME;
			}
		}
		err_msg("rename() syscall reached max retry: %d", SYSCALL_CALL_MAX_RETRY_TIMES);
		err_sys("rename(%s, %s) error", oldpath, newpath);
	}
	return ret;
}


int syscall_sleep(long ms)
{
	struct timespec ts;
	int ret;

	ts.tv_sec = 0;
	ts.tv_nsec = (ms % 1000) * 1000000;
	ret = nanosleep(&ts, NULL);
	if (ret < 0) {
		err_sys("nanosleep error");
	}
	return ret;
}
