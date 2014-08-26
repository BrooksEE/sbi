TARGET=sasmc

SRC=sasmc.c tokenizer.c
HEADERS=$(wildcard *.h)
OBJS=$(patsubst %.c, %.o, $(SRC))

all: $(TARGET) 

$(TARGET): $(OBJS)
	g++ -o $@ $^

%.o: %.c $(HEADERS)
	g++ -c -o $@ $^

clean:
	rm -f $(TARGET) $(OBJS) 
