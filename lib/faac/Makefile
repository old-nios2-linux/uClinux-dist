VER = faac-1.28
URL = http://downloads.sourceforge.net/faac/$(VER).tar.bz2

# this controls the internal mp4v2 library
CONF_OPTS = \
	--without-mp4v2

include $(ROOTDIR)/tools/autotools.mk

romfs:
	$(ROMFSINST) -d -e CONFIG_FMT_USE_FDPIC_ELF $(STAGEDIR)/usr/lib/libfaad.so.0 /usr/lib/libfaad.so.0
	$(ROMFSINST) -d -e CONFIG_FMT_USE_FDPIC_ELF $(STAGEDIR)/usr/lib/libmp4ff.so.0 /usr/lib/libmp4ff.so.0
