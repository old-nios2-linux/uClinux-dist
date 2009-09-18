CC = bfin-elf-gcc
CXX = bfin-elf-g++

ROOTDIR  ?= /opt/bfin/trunk
LINUXDIR ?= linux-2.6.x

ifdef CURRENTDIR
	LIB_COREB = $(CURRENTDIR)/dsp/dsp.a
else
	CURRENTDIR := $(shell pwd)/../..
	LIB_COREB = $(shell ls $(CURRENTDIR)/dsp/*.o)
endif

INCLUDE = -include $(ROOTDIR)/$(LINUXDIR)/include/linux/autoconf.h -I$(ROOTDIR)/$(LINUXDIR)/include/ -I$(ROOTDIR)/$(LINUXDIR)/arch/blackfin/include/ -I$(ROOTDIR)/$(LINUXDIR)/arch/blackfin/mach-bf561/include/ -I$(CURRENTDIR)/include -I$(CURRENTDIR)/demo/include

CPPFLAGS := -g3 -Wall -mcpu=bf561 -D__KERNEL__ -D__DSP__ $(INCLUDE)
CFLAGS   := -g3 -Wall -mcpu=bf561 -D__KERNEL__ -D__DSP__ $(INCLUDE)
ASFLAGS  := -g3 -Wall -mcpu=bf561 -D__KERNEL__ -D__DSP__ $(INCLUDE) -D__ASSEMBLY__
LDFLAGS  := $(CFLAGS) -mmulticore -mcoreb


all: c_demo cpp_demo

OBJS = c_demo.o $(LIB_COREB)
c_demo: $(OBJS) coreb.lds
	$(CC) -T coreb.lds $(LDFLAGS) $(OBJS) -o $@
	#bfin-elf-objdump -x -D $@ > $@.dump

CXXOBJS = cpp_demo.o $(LIB_COREB)
cpp_demo: $(CXXOBJS) coreb.lds
	$(CXX) -T coreb.lds $(LDFLAGS) $(CXXOBJS) -o $@
	#bfin-elf-objdump -x -D $@ > $@.dump


%.o: %.S
	$(CC) -c $(ASFLAGS) -o $@ $<

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

%.o: %.cpp
	$(CXX) -c $(CPPFLAGS) -o $@ $<

romfs:
	$(ROMFSINST) -d /usr/lib/firmware/c_demo
	$(ROMFSINST) -d /usr/lib/firmware/cpp_demo

clean:
	rm -f *.map *.dump *.elf *.o *.i *.s c_demo cpp_demo

.PHONY: all clean romfs
