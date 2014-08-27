CC      = gcc
CFLAGS  = -O2 -g
OBJECTS = sbasc.o
TARGET  = sbasc

$(TARGET): $(OBJECTS)

all: $(TARGET)

clean:
	rm -f $(TARGET) $(OBJECTS) test.sasm out.sbi out.sbi.prg

test: $(TARGET)
	./sbasc
	./sasmc -i test.sasm -o test.sbi
	./sbi test.sbi

