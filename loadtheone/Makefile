#
# Leendert van Duijn:
# Handwritten Makefile, does its job, no more no less.
#

SLC=slc 
SLR=slr
CFLAGS=`../gcflags`
all: sim_out

clean:
	rm -f *.o sim_out

SIMC=main_sim.c elf.c basfunc.c loader.c
SIMH=ELF.h loader.h loader_api.h basfunc.h
sim_out: $(SIMC) $(SIMH)
	$(SLC) $(CFLAGS) -b mta $(SIMC) -o sim_out
run: sim_out
	$(SLR) sim_out sim_testprog 2> ll

