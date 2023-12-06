#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include "ds_common.h"
#include "ds_err.h"

/* Read "n" bytes from a descriptor  */
ssize_t readn(int fd, void *ptr, size_t n)
{
	size_t nleft;
	ssize_t nread;

	nleft = n;
	while (nleft > 0) {
		if ((nread = read(fd, ptr, nleft)) < 0) {
			if (nleft == n)
				return (-1);	/* error, return -1 */
			else
				break;	/* error, return amount read so far */
		}
		else if (nread == 0) {
			break;	/* EOF */
		}
		nleft -= nread;
		ptr += nread;
	}
	return (n - nleft);	/* return >= 0 */
}

/* Write "n" bytes to a descriptor  */
ssize_t writen(int fd, const void *ptr, size_t n)
{
	size_t nleft;
	ssize_t nwritten;

	nleft = n;
	while (nleft > 0) {
		if ((nwritten = write(fd, ptr, nleft)) < 0) {
			if (nleft == n)
				return (-1);	/* error, return -1 */
			else
				break;	/* error, return amount written so far */
		}
		else if (nwritten == 0) {
			break;
		}
		nleft -= nwritten;
		ptr += nwritten;
	}
	return (n - nleft);	/* return >= 0 */
}

void mkalldir(char *dir, mode_t mode)
{
	char *p;
	char all_dir[PATH_MAX] = {0};
	int i;

	if (dir == NULL || *dir == '/') return;

	for (i = 0, p = dir; i <= strlen(dir); i++) {
		if (p[i] != '/' && p[i] != '\0') {
			continue;
		}
		strncpy(all_dir, dir, i);
		if (mkdir(all_dir, mode) < 0) {
			if (errno != EEXIST) {
				err_sys("mkdir(%s, %o) error", all_dir, mode);
			}
		}
	}
	return;
}

