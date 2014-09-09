SBI=../../examples/linux/sbi
SASM=../../sasmc/sasmc
SRC=ast.cpp
OBJ=$(patsubst %.cpp, %.o, $(SRC))

all: run 

%.tab.c %.tab.h: %.y ast.h
	bison -t -d $< 

lex.yy.c: sbcscript.l sbcscript.tab.h
	flex $< 

%.o: %.cpp ast.h
	g++ -c -g -o $@ $<

sbcc: lex.yy.c sbcscript.tab.c $(OBJ)
	g++ -g -o $@ $^

$(SASM): $(wildcard ../../sasmc/*.cpp ../../sasmc/*.h)
	make -C ../../sasmc
$(SBI): $(wildcard ../../examples/linux/*.c ../../examples/linux/*.h )
	make -C ../../examples/linux

%.sbi: %.sasm $(SASM)
	$(SASM) -i $< -o $@ -cl

%.sasm: %.sbc sbcc 
	./sbcc $< -d > $@

run: first.sbi $(SBI)
	$(SBI) $<
