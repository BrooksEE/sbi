
SBIDIR=../../sbi
SRC=main.c $(SBIDIR)/sbi.c
OBJS=$(patsubst %.c, %.o, $(SRC))

%.o:%.c $(SBIDIR)/sbi.h
	gcc $(CCFLAGS) -c -o $@ -I$(SBIDIR) $<

progsrc=$(wildcard programs/*.sasm)
sbis=$(patsubst %.sasm, %.sbi, $(progsrc))
SASMC=../../sasmc/sasmc

all: sbi $(sbis) 

sbi: $(OBJS)
	gcc -o $@ $^

$(SASMC): $(wildcard ../../sasmc/*.cpp) $(wildcard ../../sasmc/*.h) 
	make -C ../../sasmc

%.sbi:%.sasm $(SASMC)
	$(SASMC) -i $< -o $@ -s -cl

clean:
	rm -f $(OBJS) $(sbis)
