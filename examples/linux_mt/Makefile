all: init sbi progs
init: sbi.c sbi.h
sbi.c: ../../sbi/sbi.c
	cp ../../sbi/sbi.c sbi.c
sbi.h: ../../sbi/sbi.h
	cp ../../sbi/sbi.h sbi.h
sbi: sbi.o main.o
	gcc -o sbi sbi.o main.o
sbi.o: sbi.c sbi.h funclib.h
	gcc -c -o sbi.o sbi.c
main.o: main.c sbi.h funclib.h
	gcc -c -o main.o main.c
progs: programs/counter.sbi programs/pow.sbi programs/libtest.sbi programs/jumps.sbi programs/subroutines.sbi programs/interrupts.sbi
sasmc:
	cp ../../sasmc_linux/sasmc sasmc
programs/counter.sbi: sasmc programs/counter.sasm
	./sasmc -i programs/counter.sasm -o programs/counter.sbi -s -cl
programs/pow.sbi: sasmc programs/pow.sasm
	./sasmc -i programs/pow.sasm -o programs/pow.sbi -s -cl
programs/libtest.sbi: sasmc programs/libtest.sasm
	./sasmc -i programs/libtest.sasm -o programs/libtest.sbi -s -cl
programs/jumps.sbi: sasmc programs/jumps.sasm
	./sasmc -i programs/jumps.sasm -o programs/jumps.sbi -s -cl
programs/subroutines.sbi: sasmc programs/subroutines.sasm
	./sasmc -i programs/subroutines.sasm -o programs/subroutines.sbi -s -cl
programs/interrupts.sbi: sasmc programs/interrupts.sasm
	./sasmc -i programs/interrupts.sasm -o programs/interrupts.sbi -s -cl
clean:
	rm -f sbi.c sbi.h sbi.o main.o sbi sasmc programs/counter.sbi programs/pow.sbi programs/libtest.sbi programs/jumps.sbi programs/subroutines.sbi programs/interrupts.sbi
