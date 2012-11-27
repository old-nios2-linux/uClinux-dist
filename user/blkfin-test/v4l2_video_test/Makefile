CFLAGS += -Wall

LDLIBS += -lm

all: v4l2_video_capture v4l2_video_display v4l2_video_loopback

romfs:
	$(ROMFSINST) /bin/v4l2_video_capture
	$(ROMFSINST) /bin/v4l2_video_display
	$(ROMFSINST) /bin/v4l2_video_loopback

clean:
	rm -f v4l2_video_capture v4l2_video_display v4l2_video_loopback *.elf *.gdb *.o

.PHONY: all clean romfs
