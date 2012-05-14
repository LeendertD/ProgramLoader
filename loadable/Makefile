all: tinyex spawny printy fourtwo

CRT=crt_simple.s argroom.s envroom.s
#argroom.s
CC=slc -nostdlib -fPIC -fpic -Wl,-shared

CLEANONE=./cleanone.sh
CFLAGS=`../gcflags`
MK=./makeone.sh
clean:
	$(CLEANONE) tinyex
	$(CLEANONE) spawny
	$(CLEANONE) printy
	$(CLEANONE) fourtwo

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
