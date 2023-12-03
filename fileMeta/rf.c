#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "ds_err.h"

void show_status(int num)
{
	if (num <= 0) {
		return;
	}
	switch (num % 4) {
		case 0:
			printf(" -\r");
			fflush(NULL);
			break;
		case 1:
			printf(" \\\r");
			fflush(NULL);
			break;
		case 2:
			printf(" |\r");
			fflush(NULL);
			break;
		case 3:
			printf(" /\r");
			fflush(NULL);
			break;
	}
}

int main(int argc, char *argv[])
{
	unsigned long count, i, j, k;

	if (argc != 2) {
		err_quit("USAGE: %s count", argv[0]);
	}
	count = strtoul(argv[1], NULL, 10);

	for (i = 0; i < 1024; i++) {
		char dir_l1[4096] = { 0 };

		snprintf(dir_l1, sizeof(dir_l1), "dir_level1_%ld", i);
		if (chdir(dir_l1) < 0) {
			err_ret("chdir(%s) error, continue...", dir_l1);
			continue;
		}
		for (j = 0; j < 4096; j++) {
			char dir_l2[4096] = { 0 };

			snprintf(dir_l2, sizeof(dir_l2), "dir_level2_%ld", j);
			if (chdir(dir_l2) < 0) {
				err_ret("chdir(%s) error, continue...", dir_l2);
				continue;
			}
			for (k = 0; k < count; k = k + 2) {
				char file[4096] = { 0 };
				snprintf(file, sizeof(file), "file_%ld", k);
				if (unlink(file) < 0) {
					err_ret("unlink(%s) error, continue...", file);
					continue;
				}
			}
			if (chdir("..") < 0) {
				err_ret("chdir(..) error, continue...");
				continue;
			}
		}
		show_status(j + 1);
		if (chdir("..") < 0) {
			err_ret("chdir(..) error, continue...");
			continue;
		}
	}

	return 0;
}
