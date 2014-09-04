all: sbi progs

SBIDIR=../../sbi
SRC=main.c $(SBIDIR)/sbi.c
OBJS=$(patsubst %.c, %.o, $(SRC))

sbi: $(OBJS)
	gcc -o $@ $^

%.o:%.c $(SBIDIR)/sbi.h
	gcc -c -o $@ -I$(SBIDIR) $<

progsrc: $(wildcard programs/*.sasm ) 
progs = $(patsubst %.sasm, %.sbi, $(progsrc))
SASMC=../../sasmc/sasmc

$(SASMC): $(wildcard ../../sasmc/*.cpp) $(wildcard ../../sasmc/*.h) 
	make -C ../../sasmc

%.sbi:%.sasm $(SASMC) 
	$(SASMC) -i $< -o $@ -s -cl

clean:
	rm -f $(OBJS) $(progs) 
