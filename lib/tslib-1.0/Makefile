STAGEDIR = $(ROOTDIR)/lib/tslib-1.0/build

all: build/Makefile
	$(MAKE) -C build install DESTDIR=$(STAGEDIR)

build/Makefile:
	find . -type f -print0 | xargs -0 touch -r configure
	set -e ; \
	rm -rf build ; \
	mkdir build ; \
	cd build ; \
	../configure \
		$(CONFIGURE_OPTS) \
		--disable-h2200-linear \
		--disable-ucb1x00 \
		--disable-corgi \
		--disable-collie \
		--disable-h3600 \
		--disable-mk712 \
		--disable-arctic2 \
		--disable-shared \
		CFLAGS='-DUSE_INPUT_API' 

clean:
	rm -rf build

romfs:
	$(ROMFSINST) $(STAGEDIR)/usr/bin/ts_calibrate /bin/

.PHONY: all clean romfs
