
CONFOPTS :=	--enable-dependency-tracking		\
		--sysconfdir=/etc/config

ifdef CONFIG_USER_TRUSTEDSOURCE_V2
CONFOPTS += CPPFLAGS="-I$(ROOTDIR)/include" LIBS="-lrep -lsnapgear" LDFLAGS="-L$(ROOTDIR)/lib"
endif

all: build/build
	$(MAKE) -C build

build/build: makefile
	@chmod u+x configure
	@rm -rf build
	@mkdir build
	(cd build; sh ../configure $(CONFIGURE_OPTS) $(CONFOPTS))
	@touch build/build

clean:
	rm -rf build

romfs:
	[ -d $(ROMFSDIR)/etc/clamav ] || mkdir $(ROMFSDIR)/etc/clamav
	$(ROMFSINST) build/src/clamsmtpd /bin/clamsmtpd
	$(ROMFSINST) doc/clamsmtpd.conf /etc/clamav/clamsmtpd.conf
