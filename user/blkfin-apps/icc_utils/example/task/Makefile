CC := bfin-elf-gcc -g
LD := bfin-elf-ld

# we have to clobber a bunch of flags as they are normally geared towards
# running under linux, not a bare metal environment like core b
PWD:= $(shell pwd)
KERNELDIR := ${PWD}/../../../../../linux-2.6.x
ICC_INCLUDE := ${PWD}/../../include
ICC_CORE := ../../icc_core/icc
TASK_LOAD_BASE := 0x3c00000

CFLAGS  := -mcpu=bf561 -mmulticore -mcoreb -mlong-calls -I${ICC_INCLUDE} -I${KERNELDIR}/include -I${KERNELDIR}/arch/blackfin/include -I${KERNELDIR}/arch/blackfin/mach-bf561/include -I${KERNELDIR}/drivers/staging/icc/include -DCONFIG_BF561 -D__KERNEL__
LDFLAGS  = -L${ICC_INCLUDE}
ASFLAGS  = $(CFLAGS)
all: task1 task2

task1: task1.o $(ICC_CORE)
	$(LD) $(LDFLAGS) -o task1 -T coreb_task.lds --just-symbol $(ICC_CORE) task1.o -Ttext $(TASK_LOAD_BASE)

task2: task2.o $(ICC_CORE)
	$(LD) $(LDFLAGS) -o task2 -T coreb_task.lds --just-symbol $(ICC_CORE) task2.o -Ttext $(TASK_LOAD_BASE)

clean:
	rm -rf *.o task1 task2

.PHONY: all clean
