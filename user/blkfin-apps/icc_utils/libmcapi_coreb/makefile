VER = libmcapi-1.0
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
CFLAGS := -Wall -mcpu=bf561 -mmulticore -mcoreb -mlong-calls -DCONFIG_BF561
LDFLAGS :=
include $(ROOTDIR)/tools/autotools.mk
