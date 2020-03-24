CC	 	= gcc
LD	 	= gcc
CFLAGS	 	= -Wall -g

LDFLAGS	 	=  
DEFS 	 	=

all:	sendfile recvfile 

server: sendfile.c
	$(CC) $(DEFS) $(CFLAGS) $(LIB) -o sendfile sendfile.c

client: recvfile.c
	$(CC) $(DEFS) $(CFLAGS) $(LIB) -o recvfile recvfile.c

clean:
	rm -f *.o
	rm -f *~
	rm -f core.*.*
	rm -f sendfile
	rm -f recvfile
