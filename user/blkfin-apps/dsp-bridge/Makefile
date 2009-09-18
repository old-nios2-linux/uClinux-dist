export ROOTDIR  ?= /opt/bfin/trunk
export LINUXDIR ?= linux-2.6.x
export CURRENTDIR := $(shell pwd)

SUBDIRS = kernel dsp util demo

all: all_subdirs

clean: clean_subdirs

romfs: romfs_subdirs

%_subdirs:
	for d in $(SUBDIRS) ;do $(MAKE) -C $$d $(@:_subdirs=) || exit $$? ; done

.PHONY: all clean romfs
