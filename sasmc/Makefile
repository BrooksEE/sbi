sasmc: sasmc.o tokenizer.o
	g++ -o sasmc sasmc.o tokenizer.o
sasmc.o: sasmc.cpp tokenizer.h
	g++ -I../sbi -c -o sasmc.o sasmc.cpp
tokenizer.o: tokenizer.cpp tokenizer.h
	g++ -c -o tokenizer.o tokenizer.cpp
clean:
	rm -f sasmc sasmc.o tokenizer.o
