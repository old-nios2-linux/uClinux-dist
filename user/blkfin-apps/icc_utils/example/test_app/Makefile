CC = bfin-uclinux-gcc

# we have to clobber a bunch of flags as they are normally geared towards
# running under linux, not a bare metal environment like core b
ICC_INCLUDE := ${pwd}/../icc_core
CFLAGS  ?=
CFLAGS  += -I$(KERNELDIR)/arch/blackfin/mach-bf561/include -I$(KERNELDIR)/drivers/staging/icc/include -I$(ICC_INCLUDE) -g
LDFLAGS  = $(CFLAGS)
ASFLAGS  = $(CFLAGS)
PWD:= $(shell pwd)
KERNELDIR := ${PWD}/../../../../../linux-2.6.x

all: packet_test session_packet_test

packet_test: packet.o
	$(CC) $(CFLAGS) $< -o $@ > $@.map

session_packet_test: session_packet.o
	$(CC) $(CFLAGS) $< -o $@ > $@.map
clean:
	rm -rf *.o *.map *.gdb packet_test session_packet_test

.PHONY: all clean
