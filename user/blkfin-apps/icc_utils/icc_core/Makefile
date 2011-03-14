CC = bfin-elf-gcc -g
LD = bfin-elf-ld

# we have to clobber a bunch of flags as they are normally geared towards
# running under linux, not a bare metal environment like core b
CPPFLAGS =
CFLAGS  ?= -mcpu=bf561 -mlong-calls
CFLAGS  += -mmulticore -mcoreb -L${ICC_INCLUDE} -I${ICC_INCLUDE} -I${KERNELDIR}/include -I${KERNELDIR}/arch/blackfin/include -I${KERNELDIR}/arch/blackfin/mach-bf561/include -I${KERNELDIR}/drivers/staging/icc/include -DCONFIG_BF561 -D__KERNEL__
LDFLAGS  = $(CFLAGS)
ASFLAGS  = $(CFLAGS)
PWD:= $(shell pwd)
KERNELDIR := ${PWD}/../../../../linux-2.6.x
ICC_INCLUDE := ${PWD}/../include
all: icc

ICC_OBJS = coreb_start.o test.o corebboot.o vsprintf.o mempool.o protocol.o
icc: $(ICC_OBJS)
	$(CC) -Wl,--verbose -Wl,-M -T coreb.lds -I${KERNELINCDIR} $(LDFLAGS) $(ICC_OBJS) -o $@ > $@.map
	bfin-elf-objdump -x -D $@ > $@.dump

clean:
	rm -f *.map *.dump *.elf *.gdb *.o icc

.PHONY: all clean romfs
