VER = can-utils-rel-2.0.0-pre7

all: $(VER)/Makefile
	$(MAKE) -C $(VER)

clean:
	$(MAKE) -C $(VER) clean

romfs:
	$(ROMFSINST) $(VER)/cansend      /usr/bin/cansend
	$(ROMFSINST) $(VER)/canplayer    /usr/bin/canplayer
	$(ROMFSINST) $(VER)/cangen       /usr/bin/cangen
	$(ROMFSINST) $(VER)/cansniffer   /usr/bin/cansniffer
	$(ROMFSINST) $(VER)/candump      /usr/bin/candump
	$(ROMFSINST) $(VER)/slcan_attach /usr/bin/slcan_attach
	$(ROMFSINST) $(VER)/log2long     /usr/bin/log2long
	$(ROMFSINST) $(VER)/log2asc      /usr/bin/log2asc
	$(ROMFSINST) $(VER)/asc2log      /usr/bin/asc2log
.PHONY: all clean romfs
