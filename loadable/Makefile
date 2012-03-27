all: tinyex

CRT=crt_simple.s
CC=slc -b mta -nostdlib 

clean:
	rm -f tinyex

tinyex: tinyex.c $(CRT)
	$(CC) tinyex.c $(CRT) -o tinyex
