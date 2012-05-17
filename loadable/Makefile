all: tinyex spawny printy fourtwo sparmy

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
	$(CLEANONE) sparmy

crt_fun.o: crt_fun.c
	$(CC) $(CFLAGS) -c $<

argroom.o: argroom.c
	$(CC) $(CFLAGS) -c $<

envroom.o: envroom.c
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

NE=sparmy
$(NE): $(NE).c $(CRT)
	$(MK) $@


