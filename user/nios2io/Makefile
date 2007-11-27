#
# Makefile for nios2io
#

#LINUXDIR = ../../linux-2.6.x
EXEC	= nios2io
OBJS	= nios2io.o

CFLAGS	 += -I$(LINUXDIR)/include
FLTFLAGS += -s 1024

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(EXTRALIBS) $(LDLIBS)

romfs:
	$(ROMFSINST) /bin/$(EXEC)

clean:
	-rm -f $(EXEC) *.gdb *.elf *.o
