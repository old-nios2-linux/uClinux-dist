VER = inetutils-1.4.2

all %::
	make -C $(VER)

clean:
	make clean -C $(VER)

romfs:
	$(ROMFSINST) -d $(VER)/rcp/rcp /bin/rcp
	$(ROMFSINST) -d $(VER)/rsh/rsh /bin/rsh
	$(ROMFSINST) -d $(VER)/rshd/rshd /bin/rshd
	$(ROMFSINST) -a "shell       stream tcp nowait root /bin/rshd" /etc/inetd.conf
