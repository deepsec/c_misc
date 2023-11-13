#include <stdio.h>
#include <ds_err.h>

int main(int argc, char *argv[])
{
    FILE *src, *dst;
    int n;
    char buf[4096];

    if (argc != 3) {
        err_quit("usage: %s src dst", argv[0]);
    }    

    if ((src = fopen(argv[1], "r")) == NULL) {
        err_sys("can't open file %s", argv[1]);
    }
    if ((dst = fopen(argv[2], "w")) == NULL) {
        err_sys("can't create file %s", argv[2]);
    }

    setvbuf(src, NULL, _IOFBF, 4096);
    setvbuf(dst, NULL, _IOFBF, 4096);
    while (1) {
        if ((n = fread(buf, 1, 4096, src)) > 0) {
            fwrite(buf, 1, n, dst);
        } else {
            if (ferror(src) ||  ferror(dst)) {
                err_sys("fread/fwrite error");
            }
            break;
        }
    }    
    fflush(src);
    fflush(dst);
    fclose(src);
    fclose(dst);

    return 0;
}
