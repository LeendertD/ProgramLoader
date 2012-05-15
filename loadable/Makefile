all: tinyex spawny printy fourtwo

CRT=crt_fun.o argroom.o envroom.o

CC=slc -b mta -nostdlib -fPIC -fpic -Wl,-shared

CLEANONE=./cleanone.sh
CFLAGS=`../gcflags`
MK=./makeone.sh
clean:
	rm -rf *.o
	$(CLEANONE) tinyex
	$(CLEANONE) spawny
	$(CLEANONE) printy
	$(CLEANONE) fourtwo

crt_fun.o: crt_fun.c
	$(CC) $(CFLAGS) -c $<

argroom.o: argroom.s
	$(CC) $(CFLAGS) -c $<

envroom.o: envroom.s
	$(CC) $(CFLAGS) -c $<

NA=tinyex
$(NA): $(NA).c $(CRT)
	$(MK) $@

NB=spawny
$(NB): $(NB).c $(CRT)
	$(MK) $@
NC=printy
$(NC): $(NC).c $(CRT)
	$(MK) $@
ND=fourtwo
$(ND): $(ND).c $(CRT)
	$(MK) $@
