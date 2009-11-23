VER = madplay-0.15.2b

CONF_OPTS = \
	--without-x

include $(ROOTDIR)/tools/autotools.mk

romfs:
	$(ROMFSINST) -d $(STAGEDIR)/usr/bin/madplay /usr/bin/madplay
