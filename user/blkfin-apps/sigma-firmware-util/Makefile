EXEC = adau1761.bin

all: $(EXEC)

$(EXEC): gen_firmware
	./gen_firmware

romfs:
	$(ROMFSINST) -d /lib/firmware/$(EXEC)

clean:
	-rm -f $(EXEC) *.elf *.gdb *.o gen_firmware

gen_firmware : gen_firmware.c
	gcc ./gen_firmware.c -o gen_firmware
