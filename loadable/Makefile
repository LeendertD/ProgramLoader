all: tinyex spawny printy fourtwo

CRT=crt_simple.s argroom.s
#argroom.s
CC=slc -nostdlib -fPIC -fpic -Wl,-shared

CFLAGS=`../gcflags`
MK=./makeone.sh
clean:
	rm -f tinyex spawny printy fourtwo
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
