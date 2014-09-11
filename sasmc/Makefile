SRC=sasmc.cpp libsasmc.cpp tokenizer.cpp
OBJ=$(patsubst %.cpp, %.o, $(SRC))

sasmc: $(OBJ) 
	g++ -o $@ $^ 

%.o: %.cpp $(wildcard *.h ../sbi/*.h)
	g++ -I../sbi -c $(CCFLAGS) -o $@ $<	

clean:
	rm -f sasmc $(OBJ) 
