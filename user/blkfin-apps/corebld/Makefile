SUBDIRS = example test_module

CFLAGS += -Wall
CC = bfin-uclinux-gcc

all: corebld all_subdirs

corebld: corebld.o

clean: clean_subdirs
	rm -f corebld corebld.gdb

romfs: romfs_subdirs
	$(ROMFSINST) /bin/corebld

%_subdirs:
	for d in $(SUBDIRS) ; do $(MAKE) -C $$d $(@:_subdirs=) || exit $$? ; done

.PHONY: all clean romfs
