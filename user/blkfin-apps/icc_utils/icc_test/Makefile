CC = bfin-uclinux-gcc

# we have to clobber a bunch of flags as they are normally geared towards
# running under linux, not a bare metal environment like core b
CFLAGS  ?=
CFLAGS  += -I$(KERNELDIR)/include -I$(KERNELDIR)/arch/blackfin/mach-bf561/include -I$(KERNELDIR)/drivers/staging/icc/include
LDFLAGS  = $(CFLAGS)
ASFLAGS  = $(CFLAGS)
PWD:= $(shell pwd)
KERNELDIR := ${PWD}/../../../../linux-2.6.x

all: icc_test

icc_test: icc_test.o
	$(CC) $(LDFLAGS) icc_test.o  -o $@ > $@.map
romfs: icc_test
	$(ROMFSINST) -d /bin/icc_test
clean:
	rm -f *.map *.dump *.elf *.gdb *.o icc_test

.PHONY: all clean romfs
