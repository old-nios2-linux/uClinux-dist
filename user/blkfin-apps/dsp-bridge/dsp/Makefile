CC = bfin-elf-gcc
CXX = bfin-elf-g++

ROOTDIR  ?= /opt/bfin/trunk
LINUXDIR ?= linux-2.6.x
CURRENTDIR ?= $(shell pwd)/..

INCLUDE = -include $(ROOTDIR)/$(LINUXDIR)/include/linux/autoconf.h -I$(ROOTDIR)/$(LINUXDIR)/include/ -I$(ROOTDIR)/$(LINUXDIR)/arch/blackfin/include/ -I$(ROOTDIR)/$(LINUXDIR)/arch/blackfin/mach-bf561/include/ -I$(CURRENTDIR)/include

CFLAGS   := -g3 -Wall -mcpu=bf561 -D__KERNEL__ -D__DSP__ $(INCLUDE)
ASFLAGS  := -g3 -Wall -mcpu=bf561 -D__KERNEL__ -D__DSP__ $(INCLUDE) -D__ASSEMBLY__

SHARE_DIR := $(CURRENTDIR)/share
SHARE_FILES = $(shell ls $(SHARE_DIR))
SHARE_OBJS = $(subst .c,.o,$(filter %.c,$(SHARE_FILES)))

COREB_FILES = $(shell ls .)
COREB_OBJS = $(subst .c,.o,$(filter %.c,$(COREB_FILES)))
COREB_OBJS += $(subst .S,.o,$(filter %.S,$(COREB_FILES)))
LIB_COREB = dsp.a

all: $(SHARE_FILES) $(LIB_COREB)

$(LIB_COREB): $(SHARE_OBJS) $(COREB_OBJS)
	$(AR) cr $(LIB_COREB) $(SHARE_OBJS) $(COREB_OBJS)

$(SHARE_FILES):
	@ln -sf $(SHARE_DIR)/$@ $@

%.o: %.S
	$(CC) -c $(ASFLAGS) -o $@ $<

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<


romfs:
	$(ROMFSINST) -d $(STAGEDIR)/usr/lib/$(LIB_COREB)

clean:
	rm -f *.o $(SHARE_FILES) $(LIB_COREB);

.PHONY: all clean romfs
