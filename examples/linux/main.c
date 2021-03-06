#include <stdio.h>
#include <sbi.h>
#include <stdbool.h>

#include "funclib.h"

FILE* f1;

uint8_t getfch(PCOUNT p, void* rt)
{
	fseek (f1, p, SEEK_SET);
	uint8_t r = fgetc(f1);
    //printf ( "prog1 byte %d=%d\n", p, r );
	return r;
}

int main(int argc, char** argv)
{
	printf("\nSBI Multithreading Runtime for Linux\n\n");
	
	// Open executable
	if (argc<2) { printf("Wrong arguments\n\nUse:\n %s <program.sbi> <-i>\n\n\t-i enable interrupt 2\n", argv[0]); return 8; }
	printf("Loading %s...\n", argv[1]);
	f1 = fopen((char*)argv[1], "rb");
	if (!f1) { printf("Can't open file %s\n", argv[1]); return 1; }
	
	// Init
    sbi_context_t ctx;
    ctx.debugn=debugn;
    ctx.errorn=errorn;
    ctx.print = print;
    ctx.printd = printd;
    ctx.getfch=getfch;
    ctx.sbi_user_funcs[0] = myfunc;
    ctx.sbi_user_funcs[1] = msgbox;
    ctx.sbi_user_funcs[2] = getnum;
	void* rt = sbi_init(&ctx);
	
    int ret = sbi_begin(rt);
    if (ret) {
        printf ( "Program begin error: %d\n", ret );
        return ret;
    }
     
	printf("Running...\n");
	
    bool interrupts = argc>2 && strcmp(argv[2],"-i")==0;
    int int_cnt=0;
	while (sbi_running(rt)>0)
	{
		ret = sbi_step(rt);
        if (interrupts && (++int_cnt % 100==0))
            sbi_interrupt(2,rt);
	}
    sbi_cleanup(rt);
	
	printf("All threads terminated\n");
	
	if (ret==SBI_THREAD_EXIT) printf("Program reached end (no exit found)\n");
	else if (ret==SBI_PROG_EXIT) printf("Program exited (no errors)\n");
	else printf("Program exited (%d) see sbi.h\n", ret);
	
	fclose(f1);
	
	return (ret<=SBI_PROG_EXIT) ? 0 : 1;
}
