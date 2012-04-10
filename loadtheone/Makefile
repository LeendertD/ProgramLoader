
SLC=slc 
SLR=slr
CFLAGS=`../gcflags`
all: sim_out
#sim_testprog

clean:
	rm -f *.o sim_out
#sim_testprog

SIMC=main_sim.c elf.c basfunc.c loader.c
sim_out: $(SIMC) ELF.h
	$(SLC) $(CFLAGS) -b mta $(SIMC) -o sim_out
run: sim_out
	$(SLR) sim_out sim_testprog 2> ll
#	$(SLR) sim_out -Ws,-i sim_testprog
#sim_testprog: test_sim.c
#	$(SLC) $(CFLAGS) -b mta test_sim.c -o sim_testprog

#lex.yy.c: parser.lex
#	flex -R parser.lex
#parser: lex.yy.c
#	$(SLC) -b mta lex.yy.c -o parser
