CC := bfin-elf-gcc -g
LD := bfin-elf-ld

# we have to clobber a bunch of flags as they are normally geared towards
# running under linux, not a bare metal environment like core b
PWD:= $(shell pwd)
KERNELDIR := ${PWD}/../../../../../linux-2.6.x
ICC_INCLUDE := ${PWD}/../../include
ICC_CORE := ../../icc_core/icc
TASK_LOAD_BASE := 0x3c00000
LIBMCAPI_COREB := ${PWD}/../../libmcapi_coreb/.libs/libmcapi.a

CFLAGS  := -mcpu=bf561 -mmulticore -mcoreb -mlong-calls -I../../libmcapi_coreb -I../../libmcapi_coreb/include -I${ICC_INCLUDE} -I${KERNELDIR}/include -I${KERNELDIR}/arch/blackfin/include -I${KERNELDIR}/arch/blackfin/mach-bf561/include -I${KERNELDIR}/drivers/staging/icc/include -DCONFIG_BF561 -D__KERNEL__
LDFLAGS  = -L${ICC_INCLUDE}
ASFLAGS  = $(CFLAGS)
all: task1 task2 scalar mcapi_msg1

mcapi_msg1: mcapi_msg1.o $(ICC_CORE) $(LIBMCAPI_COREB)
	$(LD) -static $(LDFLAGS) -o mcapi_msg1 -T coreb_task.lds --just-symbol $(ICC_CORE) mcapi_msg1.o $(LIBMCAPI_COREB)  -Ttext $(TASK_LOAD_BASE)

task1: task1.o $(ICC_CORE)
	$(LD) $(LDFLAGS) -o task1 -T coreb_task.lds --just-symbol $(ICC_CORE) task1.o -Ttext $(TASK_LOAD_BASE)

task2: task2.o $(ICC_CORE)
	$(LD) $(LDFLAGS) -o task2 -T coreb_task.lds --just-symbol $(ICC_CORE) task2.o -Ttext $(TASK_LOAD_BASE)

scalar: scalar.o $(ICC_CORE)
	$(LD) $(LDFLAGS) -o scalar -T coreb_task.lds --just-symbol $(ICC_CORE) scalar.o -Ttext $(TASK_LOAD_BASE)

clean:
	rm -rf *.o task1 task2 scalar mcapi_msg1

.PHONY: all clean
