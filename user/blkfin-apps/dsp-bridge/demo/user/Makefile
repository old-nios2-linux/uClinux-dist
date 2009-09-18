CC = bfin-uclinux-gcc
CXX = bfin-uclinux-g++

ROOTDIR  ?= /opt/bfin/trunk
LINUXDIR ?= linux-2.6.x
CURRENTDIR ?= $(shell pwd)/../..

INCLUDE = -I$(CURRENTDIR)/include -I$(CURRENTDIR)/demo/include

CPPFLAGS := -g3 -Wall -mcpu=bf561 $(INCLUDE)
CFLAGS   := -g3 -Wall -mcpu=bf561 $(INCLUDE)
ASFLAGS  := -g3 -Wall -mcpu=bf561 $(INCLUDE)
LDFLAGS  += -pthread

all: user_app

clean:
	rm -f user_app *.gdb *.o

romfs:
	$(ROMFSINST) -d user_app /usr/bin/

.PHONY: all clean romfs
