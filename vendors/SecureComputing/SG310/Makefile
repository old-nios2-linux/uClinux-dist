#
#	Makefile -- Build instructions for SecureComputing SG310
#

ROMFSIMG      = $(IMAGEDIR)/ramdisk
ZIMAGE        = $(IMAGEDIR)/zImage
IMAGE         = $(IMAGEDIR)/image.bin
FLASH         = $(IMAGEDIR)/flash.bin
SQUASH_ENDIAN = -be

ROMFS_DIRS    = $(DEFAULT_ROMFS_DIRS)

DEVICES      += $(DEVICE_PTY_64)

FLASH_DEVICES = \
	boot,c,90,0 \
	ethmac,c,90,0 \
	bootarg,c,90,0 \
	image,c,90,4 \
	all,c,90,6

NOT_YET	= config,c,90,2

all:

clean: image.clean

romfs: romfs_dev romfs.dirs romfs.symlinks romfs.default romfs.rc romfs.version
	$(ROMFSINST) ../network.init.std /etc/network.init
	$(ROMFSINST) ../firewall.init.std /etc/firewall.init

image: image.configs image.dir image.bsslinuz image.cramfs image.bin image.tag image.copy
	if [ -f $(ROOTDIR)/boot/boot.bin ]; then \
		( \
			: 128k boot loader; \
			dd conv=sync if=$(ROOTDIR)/boot/boot.bin bs=128k count=1 ;\
			: 512k config space; \
			dd conv=sync if=/dev/zero bs=512k count=1 ;\
			: then the image; \
			cat $(IMAGE); \
		) > $(FLASH); \
	fi

include $(ROOTDIR)/vendors/config/config.dev
include ../vendor.mak
