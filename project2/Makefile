all: rfile sfile

rfile: common.h rfile.c
	gcc  -pthread rfile.c -o rfile

sfile: common.h sfile.c
	gcc  -pthread sfile.c -o sfile

clean:
	rm -f rfile sfile
