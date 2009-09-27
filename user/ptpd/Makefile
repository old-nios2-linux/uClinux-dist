all: src/Makefile
	$(MAKE) -C src
clean:
	$(MAKE) -C src clean
romfs:
	echo "installing ptpd"
	$(ROMFSINST) src/ptpd /bin/ptpd
.PHONY: all clean romfs
