#############################################################################
#
#	Makefile -- Build instructions for SecureComputing SG720-x86
#
#############################################################################

ZIMAGE    = $(IMAGEDIR)/bzImage
ROMFSIMG  = $(IMAGEDIR)/initrd
ROMFSIMGZ = $(IMAGEDIR)/initrd.gz
IMGBASE   = $(IMAGEDIR)/SG720-x86
ISODIR    = $(IMAGEDIR)/iso
EXTIMG    = $(IMAGEDIR)/ext.img


# the number of 512 byte blocks in config partition (4096 = 2Mb)
VENDDIR   = $(ROOTDIR)/vendors/$(CONFIG_VENDOR)/$(CONFIG_PRODUCT)

SQUASH_ENDIAN = -le

ROMFS_DIRS = $(DEFAULT_ROMFS_DIRS) boot

DEVICES += $(DEVICE_PTY_64)

FLASH_DEVICES = 

#############################################################################

all:

clean: image.clean

romfs: romfs_dev romfs.dirs romfs.symlinks romfs.default romfs.rc romfs.version
	$(ROMFSINST) ../network.init.std /etc/network.init
	$(ROMFSINST) ../firewall.init.std /etc/firewall.init
	$(ROMFSINST) -s /var/config /dev/flash/config

romfs.post:: image.dir image.i386.zimage
	$(ROMFSINST) $(ZIMAGE) /boot/vmlinuz
	$(ROMFSINST) /boot/message
	( \
	cd $(ROOTDIR)/romfs; \
	grep -v setmac < etc/rc > etc/rc.new; \
	mv etc/rc.new etc/rc; \
	)

image: image.configs image.dir image.i386.zimage image.16mb image.squashfs7z image.16mb.rm hdd-images iso-images
	dd if=/dev/zero bs=512 count=`expr 1 \\* 255 \\* 63` of=$(IMGBASE)-cfg.hdd
	printf "1,,61,-\n" | \
		/sbin/sfdisk --force -uS -C 1 -H 255 -S 63 $(IMGBASE)-cfg.hdd
	$(MAKE) CONFIG=0 CYLS=1 HEADS=255 SECTORS=63 IMAGE=$(IMGBASE)-cfg.hdd \
		hdd-vmdk
	cp *.vmx $(IMAGEDIR)/.

#############################################################################

iso-images:
	gzip -9 -f $(ROMFSIMG)
	$(MAKE) IMAGE=$(IMGBASE)-vga.iso  DEFAULT=vga   iso-image
	$(MAKE) IMAGE=$(IMGBASE)-com1.iso DEFAULT=com1  iso-image

iso-image:
	mkdir -p $(ISODIR)/boot/isolinux
	cp isolinux.bin $(ISODIR)/boot/isolinux/
	cp boot.msg $(ISODIR)/boot/isolinux/
	echo "default $(DEFAULT)" > $(ISODIR)/boot/isolinux/isolinux.cfg
	cat isolinux.cfg >> $(ISODIR)/boot/isolinux/isolinux.cfg
	cp $(ZIMAGE) $(ISODIR)/boot/isolinux/
	cp $(ROMFSIMGZ) $(ISODIR)/boot/isolinux/initrd.gz
	mkisofs -J -o $(IMAGE) -b boot/isolinux/isolinux.bin \
		-c boot/isolinux/boot.cat -no-emul-boot \
		-boot-load-size 4 -boot-info-table $(ISODIR)
	rm -rf $(ISODIR)

#############################################################################

hdd-images:
	$(MAKE) HEADS=255 SECTORS=63 CYLS=6 CONFIG=4 ext3-images

ext3-images:
	$(MAKE) IMAGE=$(IMGBASE)-vga.hdd  APPEND= hdd-image
	$(MAKE) IMAGE=$(IMGBASE)-com1.hdd APPEND="console=ttyS0,9600" hdd-image

ifdef SECTORS
TOTAL_SECTORS := $(shell expr $(CYLS) \* $(HEADS) \* $(SECTORS))
TOTAL_CONFIG  := $(shell expr $(CONFIG) \* 1024 \* 1024 / 512)
TOTAL_EXTSIZE := $(shell expr $(TOTAL_SECTORS) - 1 - $(TOTAL_CONFIG))
EXT_BLOCKS    := $(shell expr $(TOTAL_EXTSIZE) / 2)

hdd-image:
	rm -f $(IMAGE)
	rm -f $(EXTIMG) $(IMAGEDIR)/device.spec $(IMAGEDIR)/qemu.pid
	rm -rf $(IMAGEDIR)/ext3
	mkdir $(IMAGEDIR)/ext3
	(cd $(ROMFSDIR);tar cf - .)|(cd $(IMAGEDIR)/ext3;tar xf -)
	cp lilo.conf $(IMAGEDIR)/ext3/etc/lilo.conf 
	[ -z "$(APPEND)" ] || \
		echo "	append = \"$(APPEND)\"" >> $(IMAGEDIR)/ext3/etc/lilo.conf
	cd $(IMAGEDIR)/ext3/.; find dev -type f -name '@*' -print | \
		sed 's/[@,]/ /g' | while read path file type maj min; do \
			rm -f $(IMAGEDIR)/ext3/$$path/@$$file,$$type,$$maj,$$min; \
			echo "/$$path$$file	$$type 666 0 0 $$maj $$min - - -"; \
		done > $(IMAGEDIR)/device.spec
	/usr/bin/genext2fs -q -d $(IMAGEDIR)/ext3 -D $(IMAGEDIR)/device.spec \
		-b $(EXT_BLOCKS) $(EXTIMG)
	/sbin/tune2fs -i 0 -j $(EXTIMG)
	dd if=/dev/zero bs=512 count=1 of=$(IMAGE)
	printf "1,$(TOTAL_EXTSIZE),L,*\n,,61,-\n" | \
		/sbin/sfdisk --force -uS -C $(CYLS) -H $(HEADS) -S $(SECTORS) $(IMAGE)
	dd if=$(EXTIMG) conv=sync bs=`expr 512 \\* $(TOTAL_EXTSIZE)` count=1 >> $(IMAGE)
	dd if=/dev/zero bs=`expr 512 \\* $(TOTAL_CONFIG)` count=1 >> $(IMAGE)
	yes "" | \
		qemu -net none -nographic -pidfile $(IMAGEDIR)/qemu.pid \
			-append "rw root=301 console=ttyS0,9600 init=/bin/lilo" \
			-kernel $(ZIMAGE) $(IMAGE) | \
		while read t; do \
			echo "$$t"; \
			case "$$t" in \
			*panic*) kill -INT `cat $(IMAGEDIR)/qemu.pid` ;; \
		esac; \
	done
	rm -f $(EXTIMG) $(IMAGEDIR)/device.spec $(IMAGEDIR)/qemu.pid
	rm -rf $(IMAGEDIR)/ext3
	$(MAKE) hdd-vmdk IMAGE=$(IMAGE)

hdd-vmdk:
	( \
		echo "# VMWare Disk descriptor file"; \
		echo "version=1"; \
		echo "CID=ffffffff"; \
		echo "parentCID=ffffffff"; \
		echo "createType=\"monolithicFlat\""; \
		echo "# Extent description"; \
		echo "RW $(TOTAL_SECTORS) FLAT \"`basename $(IMAGE)`\" 0"; \
		echo "ddb.virtualHWVersion = \"4\""; \
		echo "ddb.adapterType = \"ide\""; \
		echo "ddb.geometry.cylinders = \"$(CYLS)\""; \
		echo "ddb.geometry.heads = \"$(HEADS)\""; \
		echo "ddb.geometry.sectors = \"$(SECTORS)\""; \
	) > $(IMAGE).vmdk
endif

#############################################################################

include $(ROOTDIR)/vendors/config/config.dev
include ../vendor.mak

