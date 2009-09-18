CC = bfin-uclinux-gcc
CXX = bfin-uclinux-g++

ROOTDIR  ?= /opt/bfin/trunk
LINUXDIR ?= linux-2.6.x
CURRENTDIR ?= $(shell pwd)/..

INCLUDE = -I$(CURRENTDIR)/include

CPPFLAGS := -g3 -Wall -mcpu=bf561 $(INCLUDE)
CFLAGS   := -g3 -Wall -mcpu=bf561 $(INCLUDE)

all: dsp_util

clean:
	rm -f dsp_util *.gdb *.o

romfs:
	$(ROMFSINST) -d /usr/bin/dsp_util

.PHONY: all clean romfs
