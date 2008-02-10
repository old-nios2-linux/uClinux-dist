#
#	Makefile -- Build instructions for RMI/XLS408
#

ROMFSIMG      = $(IMAGEDIR)/ramdisk
ZIMAGE        = $(IMAGEDIR)/zImage
IMAGE         = $(IMAGEDIR)/image.bin
SQUASH_ENDIAN = -be

ROMFS_DIRS    = $(DEFAULT_ROMFS_DIRS)

DEVICES      += $(DEVICE_PTY_64)

VENDOR_ROMFS_DIR := ../../SecureComputing

all:

clean: image.clean

image.initramfs:
	cp $(ROOTDIR)/$(LINUXDIR)/vmlinux $(IMAGEDIR)/vmlinux
	$(STRIP) $(IMAGEDIR)/vmlinux
	[ -n "$(NO_BUILD_INTO_TFTPBOOT)" ] || cp $(IMAGEDIR)/vmlinux /tftpboot/.

romfs.post::
	$(ROMFSINST) -s /lib/ld-linux.so.2 /lib/ld.so.1
	$(ROMFSINST) -s /bin/init /init
	( \
		cd $(ROOTDIR)/romfs; \
		grep -v setmac < etc/rc > etc/rc.new; \
		mv etc/rc.new etc/rc; \
		find . | cpio --quiet -o -H newc > $(IMAGEDIR)/initramfs.cpio; \
	)

romfs: romfs_dev_sudo romfs.dirs romfs.symlinks romfs.default romfs.rc romfs.version
	$(ROMFSINST) $(VENDOR_ROMFS_DIR)/network.init.std /etc/network.init
	$(ROMFSINST) $(VENDOR_ROMFS_DIR)/firewall.init.std /etc/firewall.init

image: image.configs image.dir image.cramfs image.initramfs

include $(ROOTDIR)/vendors/config/config.dev
include $(ROOTDIR)/vendors/SecureComputing/vendor.mak
