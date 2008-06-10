VER = freetype-2.3.5

all: build-$(VER)/Makefile
	$(MAKE) -C build-$(VER)
	$(MAKE) -C build-$(VER) install DESTDIR=$(STAGEDIR)

build-$(VER)/Makefile:
	find $(VER) -type f -print0 | xargs -0 touch -r $(VER)/configure
	set -e ; \
	rm -rf build-$(VER) ; \
	mkdir build-$(VER) ; \
	cd build-$(VER) ; \
	../$(VER)/configure $(CONFIGURE_OPTS)

clean:
	rm -rf build*

romfs:

.PHONY: all clean romfs
