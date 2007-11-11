# This makefile, like a few others in the uClinux distribution, relies on the
# fact that GNU make's search order is "makefile" before "Makefile."

all: Makefile
	$(MAKE) -f Makefile

Makefile:
	./configure $(CONFIGURE_OPTS) --prefix=

romfs:
	$(ROMFSINST) src/haserl /bin

# Default rule retries target in 'Makefile'
%:
	$(MAKE) -f Makefile $@

