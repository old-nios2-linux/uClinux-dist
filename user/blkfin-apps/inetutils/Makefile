VER = inetutils-1.6

CONFIGURE_INETUTILS_OPTS =  --without-included-regex --disable-libls \
			    --disable-tftpd \
			    --disable-syslogd \
			    --disable-rexecd \
			    --disable-uucpd \
			    --disable-inetd \
			    --disable-rlogin \
			    --disable-telnetd \
			    --disable-ftp \
			    --disable-ifconfig --disable-rlogind --with-ncurses-include-dir=$(STAGEDIR)


ifdef CONFIG_USER_INETUTILS_RCP
CONFIGURE_INETUTILS_OPTS += --enable-rcp
else
CONFIGURE_INETUTILS_OPTS += --disable-rcp
endif

ifdef CONFIG_USER_INETUTILS_RSH
CONFIGURE_INETUTILS_OPTS += --enable-rsh
else
CONFIGURE_INETUTILS_OPTS += --disable-rsh
endif

ifdef CONFIG_USER_INETUTILS_RSHD
CONFIGURE_INETUTILS_OPTS += --enable-rshd
else
CONFIGURE_INETUTILS_OPTS += --disable-rshd
endif


all: build-$(VER)/Makefile
	$(MAKE) -C build-$(VER)

build-$(VER)/Makefile:
	find $(VER) -type f -print0 | xargs -0 touch -r $(VER)/configure
	set -e ; \
	rm -rf build-$(VER) ; \
	mkdir build-$(VER) ; \
	cd build-$(VER) ; \
	../$(VER)/configure $(CONFIGURE_OPTS) $(CONFIGURE_INETUTILS_OPTS)

clean:
	rm -rf build*

romfs:
	$(ROMFSINST) -e CONFIG_USER_INETUTILS_RCP        build-$(VER)/rcp/rcp /bin/rcp
	$(ROMFSINST) -e CONFIG_USER_INETUTILS_RSH        build-$(VER)/rsh/rsh /bin/rsh
	$(ROMFSINST) -e CONFIG_USER_INETUTILS_RSHD       build-$(VER)/rshd/rshd /bin/rshd
	$(ROMFSINST) -e CONFIG_USER_INETUTILS_RSHD \
	    -a "shell   stream tcp nowait root /bin/rshd" /etc/inetd.conf
	$(ROMFSINST) -e CONFIG_USER_INETUTILS_RSHD \
	    -a "+ +" /root/.rhosts

.PHONY: all clean romfs
