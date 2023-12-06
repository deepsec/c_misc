#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
//#include "ds_common.h"
#include "ds_err.h"


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


int main(int argc, char *argv[])
{
	if (argc != 2) {
		err_quit("USAGE: %s dir", argv[0]);
	}

	mkalldir(argv[1], 0755);

	return 0;

}
