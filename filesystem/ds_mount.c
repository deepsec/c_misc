#include <sys/mount.h>
#include <string.h>
#include <unistd.h>
#include "ds_err.h"

int main(int argc, char *argv[])
{
    unsigned long flags;
    char *data, *fstype;
    int j, opt;

    flags = 0;
    data = NULL;
    fstype = NULL;

    while ((opt = getopt(argc, argv, "o:t:f:")) != -1) {
        switch (opt) {
            case 'o':
                data = optarg;
                break;
            case 't':
                fstype = optarg;
                break;
            case 'f':
                for (j = 0; j < strlen(optarg); j++) {
                    switch (optarg[j]) {
                        case 'b': flags |= MS_BIND;         break;
                        case 'd': flags |= MS_DIRSYNC;      break;
                        case 'l': flags |= MS_MANDLOCK;     break;
                        case 'm': flags |= MS_MOVE;         break;
                        case 'A': flags |= MS_NOATIME;      break;
                        case 'V': flags |= MS_NODEV;        break;
                        case 'D': flags |= MS_NODIRATIME;   break;
                        case 'E': flags |= MS_NOEXEC;       break;
                        case 'S': flags |= MS_NOSUID;       break;
                        case 'r': flags |= MS_RDONLY;       break;
                        case 'c': flags |= MS_REC;          break;
                        case 'R': flags |= MS_REMOUNT;      break;
                        case 's': flags |= MS_SYNCHRONOUS;  break;
                        default:   
                            err_quit("USAGE: %s [-o data] [-t type] [-f flags] source target", argv[0]);
                    }
                }
                break;
            default:
                err_quit("USAGE: %s [-o data] [-t type] [-f flags] source target", argv[0]);

        }
    }

    if (argc != optind + 2) {
        err_quit("USAGE: %s [-o data] [-t type] [-f flags] source target", argv[0]);
    }

    if (mount(argv[optind], argv[optind+1], fstype, flags, data) == -1) {
        ERR_SYS("mount error()");
    }

    return 0;
}
