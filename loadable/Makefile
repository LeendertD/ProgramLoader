all: tinyex spawny printy fourtwo

CRT=crt_simple.s
#argroom.s
CC=slc -nostdlib 

CFLAGS=`../gcflags`

clean:
	rm -f tinyex spawny printy fourtwo
NA=tinyex
$(NA): $(NA).c $(CRT)
	$(CC) $(CFLAGS) -b mta $(NA).c $(CRT) -o $(NA)

NB=spawny
$(NB): $(NB).c $(CRT)
	$(CC) $(CFLAGS) -b mta $(NB).c $(CRT) -o $(NB)
NC=printy
$(NC): $(NC).c $(CRT)
	$(CC) $(CFLAGS) -b mta $(NC).c $(CRT) -o $(NC)
ND=fourtwo
$(ND): $(ND).c $(CRT)
	$(CC) $(CFLAGS) -b mta $(ND).c $(CRT) -o $(ND)
