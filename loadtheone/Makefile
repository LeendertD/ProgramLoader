
SLC=slc
SLR=slr
all: sim_out sim_testprog
clean:
	rm -f *.o sim_out sim_testprog

SIMC=main_sim.c elf.c basfunc.c
sim_out: $(SIMC) ELF.h
	$(SLC) -b mta $(SIMC) -o sim_out
run: sim_out
	$(SLR) sim_out sim_testprog 2> ll
#	$(SLR) sim_out -Ws,-i sim_testprog
sim_testprog: test_sim.c
	$(SLC) -b mta test_sim.c -o sim_testprog

