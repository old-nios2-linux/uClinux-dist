#
# Makefile
#
# Version:	$Id: Makefile,v 1.4.4.2.2.2 2006/02/04 13:48:03 nbk Exp $
#

include ../Make.inc

SUBDIRS		= examples rfc
WHAT_TO_MAKE	= all

all:
	@$(MAKE) $(MFLAGS) WHAT_TO_MAKE=$@ common

clean:
	@$(MAKE) $(MFLAGS) WHAT_TO_MAKE=$@ common
	@rm -f *~

install:
	$(INSTALL) -d -m 755 $(R)$(datadir)/doc
	$(INSTALL) -d -m 755 $(R)$(datadir)/doc/freeradius
	for file in *[!~]; do \
		if [ -f $$file -a $$file != Makefile ]; then \
			$(INSTALL) -m 644 $$file $(R)$(datadir)/doc/freeradius; \
		fi; \
	done
	@$(MAKE) $(MFLAGS) WHAT_TO_MAKE=$@ common

common: $(SUBDIRS)

$(SUBDIRS):
	@echo "Making $(WHAT_TO_MAKE) in $@..."
	$(MAKE) $(MFLAGS) -C $@ $(WHAT_TO_MAKE)

.PHONY: all clean install common $(SUBDIRS)
