#include <sys/mount.h>
#include <string.h>
#include <unistd.h>
#include "ds_err.h"

void usage(const char *s)
{
     err_quit("USAGE: %s [-f flags] directory", s);
}

int main(int argc, char *argv[])
{
    unsigned long flags;
    int j, opt;

    flags = 0;
    while ((opt = getopt(argc, argv, "f:")) != -1) {
        switch (opt) {
            case 'f':
                for (j = 0; j < strlen(optarg); j++) {
                    switch (optarg[j]) {
                        case 'd': flags |= MNT_DETACH;      break;
                        case 'e': flags |= MNT_EXPIRE;      break;
                        case 'f': flags |= MNT_FORCE;       break;
                        case 'n': flags |= UMOUNT_NOFOLLOW; break;
                        default: 
                            usage(argv[0]);  
                           
                    }
                }
                break;
            default:
                usage(argv[0]);  

        }
    }

    if (argc != optind + 1) {
        usage(argv[0]);  
    }

    if (umount2(argv[optind], flags) == -1) {
        ERR_SYS("umount error()");
    }

    return 0;
}
