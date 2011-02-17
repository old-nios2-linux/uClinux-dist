CC = bfin-uclinux-gcc

# we have to clobber a bunch of flags as they are normally geared towards
# running under linux, not a bare metal environment like core b
ICC_INCLUDE := ${pwd}/../icc_core
CFLAGS  ?=
CFLAGS  += -I$(KERNELDIR)/drivers/staging/icc/include -I$(ICC_INCLUDE) -g
LDFLAGS  = $(CFLAGS)
ASFLAGS  = $(CFLAGS)
PWD:= $(shell pwd)
KERNELDIR := ${PWD}/../../../../linux-2.6.x

all: icc_loader

icc_loader: icc_loader.o
	$(CC) $(CFLAGS) icc_loader.o  -o $@ > $@.map
clean:
	rm -rf *.o *.map *.gdb icc_loader

.PHONY: all clean
