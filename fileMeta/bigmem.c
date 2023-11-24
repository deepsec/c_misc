#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "ds_err.h"

#define MEM_SIZE_1M (1024 * 1024)

int main(int argc, char *argv[])
{
    char **mem_ptr;
    unsigned long count, seconds, i;

    if (argc != 3) {
        err_quit("USAGE: %s mem_size[M] sleep_time[seconds]", argv[0]);
    }
    count = strtoul(argv[1], NULL, 10); 
    seconds = strtoul(argv[2], NULL, 10);

    mem_ptr = (char **)malloc(count * sizeof(char *));
    if (mem_ptr == NULL) {
        err_sys("malloc error");
    }
    memset(mem_ptr, 0, count * sizeof(char *));
    printf("you want to get [%lu]M memory, sleep[%lu]s: \n", count, seconds); 
    for (i = 0; i < count; i++) {
        if ((mem_ptr[i] = (char *)malloc(MEM_SIZE_1M)) == NULL) {
            err_msg("malloc error: %ldM", i);
            break;
        }
        memset(mem_ptr[i], 0, MEM_SIZE_1M);
        
        switch (i % 4) {
            case 0: printf("-\r"); break;
            case 1: printf("\\\r"); break;
            case 2: printf("|\r"); break;
            case 3: printf("/\r"); break;
        }
    }
    printf("\nmalloced: %ldM\n", (i));
    if (seconds > 0) {
        sleep(seconds);
    }
    for (i = 0; i < count; i++) {
        if (mem_ptr[i] != NULL) {
            free(mem_ptr[i]);
        }
    }
    free(mem_ptr);

    return 0;
}
