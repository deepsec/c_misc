#ifndef __DS_SYSCALL_H__
#define __DS_SYSCALL_H__

//#define _POSIX_C_SOURCE 199309L
#include <time.h>

#define SYSCALL_SLEEP_MS	  10
#define SYSCALL_INVOKE_TIMES  10

void mkalldir(char *dir, mode_t mode);

int syscall_sleep(long ms);
int sleep_open(const char *pathname, int flags, mode_t mode);
int sleep_rename(const char *oldpath, const char *newpath);

#endif

