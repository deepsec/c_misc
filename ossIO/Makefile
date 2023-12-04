CC := gcc
CFLAGS := -Wall -g -D__DS_DBG__ -D_FILE_OFFSET_BITS=64
INCLUDE := -I. -I../common
LIBS := -L/usr/local/lib -L/usr/lib -L. -L../common -pthread

COMMON_SRCS := ../common/ds_err.c
COMMON_OBJS := $(subst .c,.o, $(COMMON_SRCS))

EXEC := ossIOWrite

all: $(EXEC)
	
ds_mount: ds_mount.o $(COMMON_OBJS)
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $^ $(LIBS)

ossIOWrite: ossIOWrite.o $(COMMON_OBJS)
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $^ $(LIBS)

testso: test.o
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $^ $(LIBS) -ldeepcommon

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@ 





.PHONY: all clean
clean:
	@rm -f *.o lib*.so lib*.a $(EXEC) $(COMMON_OBJS)


## file dependency
../common/ds_err.o: ../common/ds_err.c ../common/ds_err.h
