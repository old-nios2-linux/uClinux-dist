
EXEC = servfox
OBJS = server.o spcav4l.o utils.o tcputils.o 

SERVFLAGS= -O2 -DLINUX

SERVLIBS= -lpthread

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(LDFLAGS) $(SERVFLASG) -o $@ $(OBJS) $(LDLIBS) $(SERVLIBS)

romfs:
	$(ROMFSINST) /bin/$(EXEC)

clean:
	-rm -f $(EXEC) *.elf *.gdb *.o

	
server.o:	server.c
		$(CC) $(SERVFLAGS) -c -o $@ $<
	
spcav4l.o:	spcav4l.c spcav4l.h
		$(CC) $(SERVFLAGS) -c -o $@ $<
		
utils.o:	utils.c utils.h
		$(CC) $(SERVFLAGS) -c -o $@ $<

tcputils.o:	tcputils.c tcputils.h
		$(CC) $(SERVFLAGS) -c -o $@ $<
				
