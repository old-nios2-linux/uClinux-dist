# Code to update a git snapshot from a gitweb page.
# This needs GITWEB and GITREPO set to the appropriate values:
#	GITWEB = http://git.infradead.org/?p=$(GITREPO).git
#	GITREPO = mtd-utils

gitweb-update:
	set -e ; \
	tar="$(dir $(GITWEB))`wget -q -O - '$(GITWEB)' | sed -n '/snapshot/{s:.*href="\([^"]*\).*:\1:;p;q}'`" ; \
	ver=`echo $$tar | sed 's:.*h=\([^;]*\).*:\1:'` ; \
	test "$(VER)" = "$$ver" && exit 0 ; \
	wget $$tar -O $(GITREPO).tar.gz ; \
	tar xf $(GITREPO).tar.gz ; \
	rm -rf $$ver ; \
	mv $(GITREPO) $$ver ; \
	rm $(GITREPO).tar.gz ; \
	sed -i "/^VER/s:=.*:= $$ver:" Makefile

.PHONY: gitweb-update
