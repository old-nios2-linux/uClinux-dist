CC := bfin-uclinux-gcc

MAKEARCH_KERNEL ?= $(MAKE) ARCH=blackfin CROSS_COMPILE=bfin-uclinux-
ROOTDIR  ?= $$PWD/../../..
LINUXDIR ?= linux-2.6.x

ifneq ($(LINUXDIR),)
MAKE_KERNEL = CFLAGS="" CPPFLAGS="" LDFLAGS="" \
	$(MAKEARCH_KERNEL) -C $(ROOTDIR)/$(LINUXDIR) SUBDIRS=$$PWD
else
MAKE_KERNEL = echo
endif

obj-m := nmi_wdt_test.o

all: module

module:
	$(MAKE_KERNEL) modules

clean:
	$(MAKE_KERNEL) clean

romfs:
	$(ROMFSINST) -d -M nmi_wdt_test.ko nmi_wdt_test.ko

.PHONY: all clean module romfs
