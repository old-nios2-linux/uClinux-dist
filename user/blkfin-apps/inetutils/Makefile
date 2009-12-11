VER = inetutils-1.6

CONF_OPTS = \
	--without-included-regex \
	--disable-libls \
	--disable-servers --disable-clients \
	--with-ncurses-include-dir=$(STAGEDIR) \
	$(call USE_ENABLE,USER_INETUTILS_RCP,rcp) \
	$(call USE_ENABLE,USER_INETUTILS_RSH,rsh) \
	$(call USE_ENABLE,USER_INETUTILS_RSHD,rshd)

include $(ROOTDIR)/tools/autotools.mk

romfs:
	$(ROMFSINST) -e CONFIG_USER_INETUTILS_RCP        build-$(VER)/rcp/rcp /bin/rcp
	$(ROMFSINST) -e CONFIG_USER_INETUTILS_RSH        build-$(VER)/rsh/rsh /bin/rsh
	$(ROMFSINST) -e CONFIG_USER_INETUTILS_RSHD       build-$(VER)/rshd/rshd /bin/rshd
	$(ROMFSINST) -e CONFIG_USER_INETUTILS_RSHD \
	    -a "shell   stream tcp nowait root /bin/rshd" /etc/inetd.conf
	$(ROMFSINST) -e CONFIG_USER_INETUTILS_RSHD \
	    -a "+ +" /root/.rhosts
