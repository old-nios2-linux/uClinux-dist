NXLIB = nxlib-0.45
FLTK = fltk-1.1.8rc3

CONF_OPTS = \
	--x-includes=$(STAGEDIR)/usr/include \
	--x-libraries=$(STAGEDIR)/usr/lib

all: fltk 
	
fltk: $(FLTK)/makeinclude
	$(MAKE) -C $(FLTK)
	DESTDIR=$(STAGEDIR) \
	$(MAKE) -C $(FLTK) install

$(FLTK)/makeinclude: nxlib
	find $(FLTK) -type f -print0 | xargs -0 touch -r $(FLTK)/configure
	set -e ; \
	cd $(FLTK) ; \
	X_EXTRA_LIBS="-lnano-X -lpthread" \
	./configure $(CONFIGURE_OPTS) $(CONF_OPTS)

nxlib: 
	$(MAKE) -C $(NXLIB)
	$(MAKE) -C $(NXLIB) install

clean:
	$(MAKE) -C $(FLTK) distclean
	$(MAKE) -C $(NXLIB) clean

romfs:	
	$(ROMFSINST) -e CONFIG_USER_MICROWIN_NXLIB -d $(FLTK)/test/editor /usr/bin/editor
	
.PHONY: all clean romfs fltk nxlib
