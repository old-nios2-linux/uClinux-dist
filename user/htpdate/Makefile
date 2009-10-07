VERS = htpdate-1.0.4

all:
	$(MAKE) -e -C $(VERS)

romfs:
	$(ROMFSINST) -d $(VERS)/httping /usr/bin/httping
	cp $(VERS)/httping.1 $(STAGEDIR)/usr/share/man/man1/httping.1

clean:
	$(MAKE) -C $(VERS) clean
	rm -f $(VERS)/*.gdb

