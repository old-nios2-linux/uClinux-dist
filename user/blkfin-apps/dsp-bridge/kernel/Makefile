ifeq ($(origin CC),default)
CC := bfin-uclinux-gcc
endif
MAKEARCH_KERNEL ?= $(MAKE) ARCH=blackfin CROSS_COMPILE=bfin-uclinux-

ROOTDIR  ?= /opt/bfin/trunk
LINUXDIR ?= linux-2.6.x
CURRENTDIR ?= $(PWD)/..

EXTRA_CFLAGS := -I$(CURRENTDIR)/include

# avoid infinite recursion
ifneq ($(LINUXDIR),)
MAKE_KERNEL = CFLAGS="" CPPFLAGS="" LDFLAGS="" \
	$(MAKEARCH_KERNEL) V=1 -C $(ROOTDIR)/$(LINUXDIR) SUBDIRS=$$PWD
else
MAKE_KERNEL = echo
endif

all: module

SHARE_DIR := $(CURRENTDIR)/share
SHARE_FILES = $(shell ls $(SHARE_DIR))
SHARE_OBJS = $(subst .c,.o,$(filter %.c,$(SHARE_FILES)))

obj-m := bridge.o
bridge-y := dsp_bridge.o $(SHARE_OBJS)

module:
	for f in $(SHARE_DIR)/*.[ch]; do ln -sf $$f  `basename $$f`;done
	$(MAKE_KERNEL) modules

clean:
	$(MAKE_KERNEL) clean
	rm -f $(SHARE_FILES)

romfs:
	$(ROMFSINST) -M -d bridge.ko coreb/bridge.ko 

.PHONY: all clean module romfs
