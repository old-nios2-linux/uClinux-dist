CC = bfin-elf-gcc -g
LD = bfin-elf-ld

# we have to clobber a bunch of flags as they are normally geared towards
# running under linux, not a bare metal environment like core b
CPPFLAGS =
CFLAGS  ?= -mcpu=bf561
CFLAGS  += -mmulticore -mcoreb -I${KERNELDIR}/include -I${KERNELDIR}/arch/blackfin/include -I${KERNELDIR}/arch/blackfin/mach-bf561/include -I${KERNELDIR}/drivers/staging/icc/include -DCONFIG_BF561 -D__KERNEL__
LDFLAGS  = $(CFLAGS)
ASFLAGS  = $(CFLAGS)
PWD:= $(shell pwd)
KERNELDIR := ${PWD}/../../../linux-2.6.x
TASK_LOAD_ADDR := 0xFEB0C000

all: icc icc_testcase icc_task

ICC_OBJS = coreb_start.o test.o corebboot.o vsprintf.o mempool.o protocol.o
icc: $(ICC_OBJS) bf561.lds coreb.lds
	$(CC) -Wl,--verbose -Wl,-M -T coreb.lds -I${KERNELINCDIR} $(LDFLAGS) $(ICC_OBJS) -o $@ > $@.map
	bfin-elf-objdump -x -D $@ > $@.dump

icc_testcase:
	$(MAKE) -C icc_test

icc_task:
	$(CC) $(CFLAGS) -c test_task.c
	$(LD) -o task1 -T coreb.lds --just-symbol icc test_task.o -Ttext $(TASK_LOAD_ADDR)

romfs: icc
	$(ROMFSINST) /bin/icc
	$(MAKE) -C icc_test romfs

clean:
	rm -f *.map *.dump *.elf *.gdb *.o icc

.PHONY: all clean romfs
