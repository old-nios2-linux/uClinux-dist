ifeq ($(origin CC),default)
CC := bfin-uclinux-gcc
endif
MAKEARCH_KERNEL ?= $(MAKE) ARCH=blackfin CROSS_COMPILE=bfin-uclinux-
ROOTDIR  ?= $(PWD)/../../../
LINUXDIR ?= linux-2.6.x

# avoid infinite recursion
ifneq ($(LINUXDIR),)
MAKE_KERNEL = CFLAGS="" CPPFLAGS="" LDFLAGS="" \
	$(MAKEARCH_KERNEL) -C $(ROOTDIR)/$(LINUXDIR) SUBDIRS=$$PWD
else
MAKE_KERNEL = echo
endif

obj-m := bfin_timer_latency.o

EXTRA_CFLAGS += -Wall -g -O2

all: module

module:
	$(MAKE_KERNEL) modules

IP = bfin
RSH = rsh -l root $(IP)
rcp: module
	rcp bfin_timer_latency.ko root@$(IP):/
	-$(RSH) rmmod bfin_timer_latency
	$(RSH) insmod /bfin_timer_latency.ko

romfs:
	$(ROMFSINST) -d -M bfin_timer_latency.ko blkfindrvs/bfin_timer_latency.ko

clean:
	$(MAKE_KERNEL) clean

.PHONY: all module romfs clean
