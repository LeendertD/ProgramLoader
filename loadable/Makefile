all: tinyex spawny printy fourtwo sparmy null sec

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
	$(CLEANONE) null
	$(CLEANONE) sec
	$(CLEANONE) hworld

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

Nf=null
$(Nf): $(Nf).c $(CRT)
	$(MK) $@


Ng=sec
$(Ng): $(Ng).c $(CRT)
	$(MK) $@
