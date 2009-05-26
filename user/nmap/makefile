#
# build nmap in a subdir
#

VER = nmap-4.20

CONFOPTS =	--without-nmapfe		\
		--includedir=$(ROOTDIR)/include	\
		--libdir=$(ROOTDIR)/lib/libpcap	\
		--with-pcap=linux		\
		--with-openssl=$(ROOTDIR)	\
		--datadir=/etc

CXXFLAGS    += -I$(STL_INCDIR) 

all: build/build
	$(MAKE) -C build

build/build: makefile
	@chmod u+x $(VER)/configure
	@rm -rf build
	@mkdir build
	(cd $(VER) && find . -type d) > build/.dirs
	(cd $(VER) && find . ! -type d | grep -v ./makefile) > build/.files
	while read t; do mkdir -p build/$$t; done < build/.dirs
	while read t; do ln -s `pwd`/$$t build/$$t; done < build/.files
	(cd build; CXXPROG="UNUSED" sh ../$(VER)/configure $(CONFIGURE_OPTS) $(CONFOPTS))
	@touch build/build

clean:
	-rm -rf build

romfs:
	[ -d $(ROMFSDIR)/etc/nmap ] || mkdir $(ROMFSDIR)/etc/nmap
	$(ROMFSINST) build/nmap			/bin/nmap
	$(ROMFSINST) nmap-services		/etc/nmap
	$(ROMFSINST) nmap-rpc			/etc/nmap
	$(ROMFSINST) nmap-os-db			/etc/nmap
	$(ROMFSINST) nmap-protocols		/etc/nmap
	$(ROMFSINST) nmap-mac-prefixes		/etc/nmap
	$(ROMFSINST) nmap-os-fingerprints	/etc/nmap
	$(ROMFSINST) nmap-service-probes	/etc/nmap


