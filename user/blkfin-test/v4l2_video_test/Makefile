CFLAGS += -Wall

LDLIBS += -lm

all: v4l2_video_test

romfs:
	$(ROMFSINST) /bin/v4l2_video_test

clean:
	rm -f v4l2_video_test *.elf *.gdb *.o

.PHONY: all clean romfs
