TARGET=sbi
SASMC=../../sasmc/sasmc

SRC=../../sbi/sbi.c main.c
OBJS=$(patsubst %.c, %.o, $(SRC))
PROGS=$(wildcard programs/*.sasm)
SBIS=$(patsubst %.sasm, %.sbi, $(PROGS))

HEADERS=../../sbi/sbi.h pclib.h
INCLUDES=../../sbi

all: $(TARGET) $(SBIS)

$(TARGET): $(OBJS)
	g++ -o $@ $^

%.o: %.c $(HEADERS)
	g++ -c -I$(INCLUDES) -o $@ $<

%.sbi: %.sasm $(TARGET)
	$(SASMC) -i $< -o $@ -cl

clean:
	rm -f $(TARGET) $(OBJS) $(SBIS) 



