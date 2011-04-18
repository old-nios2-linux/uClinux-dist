ICC_TOP_DIR := $(shell pwd)
all:   icc  icc_loader  icc_example
icc_example: icc

icc: icc_core
	$(MAKE) -C  icc_core

icc_loader: 
	$(MAKE) -C  $@ 

icc_example:
	$(MAKE) -C example/task
	$(MAKE) -C example/test_app

romfs: icc_core/icc icc_loader/icc_loader example/task/task1 example/test_app/packet_test
	$(ROMFSINST) -d icc_core/icc /bin/icc
	$(ROMFSINST) -d icc_loader/icc_loader /bin/icc_loader
	$(ROMFSINST) -d example/task/task1 /bin/task1
	$(ROMFSINST) -d example/test_app/packet_test /bin/packet_test

clean:
	$(MAKE) -C icc_core clean
	$(MAKE) -C icc_loader clean
	$(MAKE) -C example/task clean
	$(MAKE) -C example/test_app clean

.PHONY: all clean romfs icc_loader
