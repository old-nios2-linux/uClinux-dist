ICC_TOP_DIR := $(shell pwd)
all: icc icc_loader icc_example
icc: icc_core
	$(MAKE) -C icc_core

iccloader:
	$(MAKE) -C icc_loader

icc_example:
	$(MAKE) -C example/task

romfs: icc_core/icc icc_loader/icc_loader
	$(ROMFSINST) -d icc_core/icc /bin/icc
	$(ROMFSINST) -d icc_loader/icc_loader /bin/icc_loader

clean:
	$(MAKE) -C icc_core clean
	$(MAKE) -C icc_loader clean
	$(MAKE) -C example/task clean

.PHONY: all clean romfs
