
DHCP_ISC_CFLAGS         = $(filter-out -fno-common,$(CFLAGS))
DHCP_ISC_CONF_OPTS      = "ac_cv_file__dev_random=yes"

all: build/Makefile 
	$(MAKE) -C build all 

build/Makefile: makefile
	mkdir -p build
	(cd build; sh ../configure $(CONFIGURE_OPTS) $(DHCP_ISC_CONF_OPTS) CFLAGS="$(DHCP_ISC_CFLAGS)" )

clean:
	rm -rf build

romfs:
	$(ROMFSINST) build/client/dhclient /bin/dhclient
	$(ROMFSINST) build/relay/dhcrelay  /bin/dhcrelay
	$(ROMFSINST) build/server/dhcpd    /bin/dhcpd
