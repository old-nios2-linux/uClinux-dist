VER = libmcapi-2.0
CONFIGURE_BUILD := $(shell sh $(ROOTDIR)/tools/config.guess)
CONFIGURE_OPTS := \
	--host=bfin-elf \
	--build=$(CONFIGURE_BUILD) \
	--prefix=/usr \
	--sysconfdir=/etc \
	--datadir=/usr/share \
	--mandir=/usr/share/man \
	--infodir=/usr/share/info \
	--localstatedir=/var/lib \
	--disable-dependency-tracking \
	--enable-fast-install

CC := bfin-elf-gcc -mcpu=bf561
CFLAGS := -Wall -mcpu=bf561 -mmulticore -mcoreb -mlong-calls -DCONFIG_BF561 -I$(ROOTDIR)/linux-2.6.x/drivers/staging/icc/include -I$(ROOTDIR)/linux-2.6.x/arch/blackfin/mach-bf561/include/mach -I$(ROOTDIR)/user/blkfin-apps/icc_utils/include
LDFLAGS :=
include $(ROOTDIR)/tools/autotools.mk
