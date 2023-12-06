#ifndef __DS_COMMON_H__
#define __DS_COMMON_H__

ssize_t readn(int fd, void *ptr, size_t n);
ssize_t writen(int fd, const void *ptr, size_t n);
void mkalldir(char *dir, mode_t mode);


#endif
