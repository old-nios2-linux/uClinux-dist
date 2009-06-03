GITWEB = http://git.xiph.org/?p=$(GITREPO).git;a=summary
GITREPO = speex
VER = 29b1230c7fe8191ef128a6b59e6d9a633863d3bb

CONF_OPTS = \
	--without-ogg \
	--enable-blackfin-asm \
	--enable-fixed-point

include $(ROOTDIR)/tools/autotools.mk
include $(ROOTDIR)/tools/gitweb.mk

romfs:
	$(ROMFSINST) -d -e CONFIG_FMT_USE_FDPIC_ELF $(STAGEDIR)/usr/lib/libspeex.so.1 /usr/lib/libspeex.so.1
	$(ROMFSINST) -d -e CONFIG_FMT_USE_FDPIC_ELF $(STAGEDIR)/usr/lib/libspeexdsp.so.1 /usr/lib/libspeexdsp.so.1
