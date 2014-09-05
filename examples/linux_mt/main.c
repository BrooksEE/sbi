#include <stdio.h>
#include <sbi.h>

#include "funclib.h"

FILE* f1;

byte getfch(PCOUNT p, void* rt)
{
	fseek (f1, p, SEEK_SET);
	byte r = fgetc(f1);
    //printf ( "prog1 byte %d=%d\n", p, r );
	return r;
}

int main(int argc, char** argv)
{
	printf("\nSBI Multithreading Runtime for Linux\n\n");
	
	// Open executable
	if (argc!=2) { printf("Wrong arguments\n\nUse:\n %s <program.sbi>\n", argv[0]); return 8; }
	printf("Loading %s...\n", argv[1]);
	f1 = fopen((char*)argv[1], "rb");
	if (!f1) { printf("Can't open file %s\n", argv[1]); return 1; }
	
	// Init
    sbi_context_t ctx;
    ctx.debugn=debugn;
    ctx.errorn=errorn;
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
	
	while (sbi_running(rt)>0)
	{
		ret = sbi_step(rt);
	}
    sbi_cleanup(rt);
	
	printf("All threads terminated\n");
	
	if (ret==1) printf("Program reached end (no exit found)\n");
	if (ret==2) printf("Program exited (no errors)\n");
	if (ret==3) printf("Program exited (wrong instruction code)\n");
	if (ret==4) printf("Program exited (can't understand byte)\n");
	if (ret==5) printf("Program exited (user error)\n");
	
	fclose(f1);
	
	return 0;
}
