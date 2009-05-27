VER = iproute2-2.6.29-1

all: $(VER)/Config
	$(MAKE) -C $(VER) CC="$(CC)" LIBUTIL=

$(VER)/Config: Config
	cp $< $@

clean:
	$(MAKE) -C $(VER) distclean

romfs:
	$(ROMFSINST) $(VER)/etc /etc
	$(ROMFSINST) $(VER)/ip/ip /bin/ip
	$(ROMFSINST) $(VER)/misc/ss /bin/ss
	$(ROMFSINST) $(VER)/tc/tc /bin/tc

.PHONY: all clean romfs
