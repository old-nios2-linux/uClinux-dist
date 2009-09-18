SUBDIRS = dsp user

all: all_subdirs

clean: clean_subdirs

romfs: romfs_subdirs

%_subdirs:
	for d in $(SUBDIRS) ;do $(MAKE) -C $$d $(@:_subdirs=) || exit $$?; done

.PHONY: all clean romfs
