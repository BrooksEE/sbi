#include <stdio.h>
#include <sbi.h>

#include "funclib.h"

FILE* f;

int pos;

byte getfch(PCOUNT p, void* rt)
{
    fseek(f,p, SEEK_SET);
	return fgetc(f); 
}

int main(int argc, char** argv)
{
	printf("\nSBI Runtime for Linux\n\n");
	
	// Open executable
	if (argc!=2) { printf("Wrong arguments\n\nUse:\n %s <program.sbi>\n", argv[0]); return 8; }
	printf("Loading %s...\n", argv[1]);
	f = fopen((char*)argv[1], "rb");
	
	if (!f) { printf("Can't open file!\n"); return 1; }
	
	// Init_
    sbi_context_t ctx;
    ctx.debugn=debugn;
    ctx.errorn=errorn;
    ctx.getfch=getfch;
    ctx.sbi_user_funcs[0] = myfunc;
    ctx.sbi_user_funcs[1] = msgbox;
    ctx.sbi_user_funcs[2] = getnum;
    

	void* rt = sbi_init(&ctx); pos=0;
	
	int ret = sbi_begin(rt);
	if (ret==1) printf("Initialization error (no function pointers)\n");
	if (ret==2) printf("Initialization error (old format version)\n");
	if (ret==3) printf("Initialization error (invalid program file)\n");
	if (ret>3) printf("Initialization error (unknow: %i)\n", ret);
	
	if (ret>0) return 1;
	
	printf("Running...\n");
	
	while (sbi_running(rt)>0)
	{
		ret = sbi_step(rt);
	}
	
	fclose(f);
	
	if (ret==1) printf("Program reached end (no exit found)\n");
	if (ret==2) printf("Program exited (no errors)\n");
	if (ret==3) printf("Program exited (wrong instruction code)\n");
	if (ret==4) printf("Program exited (can't understand byte)\n");
	if (ret==5) printf("Program exited (user error)\n");
	
	if (ret<2) return 0; else return 1;
}
