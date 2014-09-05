#include <stdio.h>
#include <sbi.h>

#include "funclib.h"

FILE* f1;
FILE* f2;

byte getfch1(PCOUNT p, struct sbi_context_t *ctx)
{
	fseek (f1, p, SEEK_SET);
	byte r = fgetc(f1);
    //printf ( "prog1 byte %d=%d\n", p, r );
	return r;
}

byte getfch2(PCOUNT p, struct sbi_context_t *ctx)
{
	fseek (f2, p, SEEK_SET);
	byte r = fgetc(f2);
    //printf ( "prog2 byte %d=%d\n", p, r );
	return r;
}

int main(int argc, char** argv)
{
	printf("\nSBI Multithreading Runtime for Linux\n\n");
	
	// Open executable
	if (argc!=3) { printf("Wrong arguments\n\nUse:\n %s <program.sbi> <program2.sbi>\n", argv[0]); return 8; }
	printf("Loading %s...\n", argv[1]);
	f1 = fopen((char*)argv[1], "rb");
	if (!f1) { printf("Can't open file %s\n", argv[1]); return 1; }
    f2 = fopen((char*)argv[2], "rb");
    if (!f2) { printf("Can't open file %s\n", argv[2]); return 1; }
	
	// Init
    struct sbi_context_t ctx;
    ctx.debugn=debugn;
    ctx.errorn=errorn;
    ctx.sbi_user_funcs[0] = myfunc;
    ctx.sbi_user_funcs[1] = msgbox;
    ctx.sbi_user_funcs[2] = getnum;
	_sbi_init(&ctx);
	
	SBISTREAM* s1 = _sbi_createstream(getfch1);
	SBISTREAM* s2 = _sbi_createstream(getfch2);
	
	SBITHREAD* t1 = _sbi_createthread(s1);
	SBITHREAD* t2 = _sbi_createthread(s2);
	
	int ret = _sbi_loadthread(t1, &ctx);
	if (ret==-1) printf("T1: Initialization error (no function pointers)\n");
	if (ret==-2) printf("T1: Initialization error (old format version)\n");
	if (ret==-3) printf("T1: Initialization error (invalid program file)\n");
	if (ret==-4) printf("T1: Initialization error (thread number overflow)\n");
	if (ret<-4) printf("T1: Initialization error (unknow: %i)\n", ret);
	
	if (ret<0) return 1;

	ret = _sbi_loadthread(t2, &ctx);
	if (ret==-1) printf("T2: Initialization error (no function pointers)\n");
	if (ret==-2) printf("T2: Initialization error (old format version)\n");
	if (ret==-3) printf("T2: Initialization error (invalid program file)\n");
	if (ret==-4) printf("T2: Initialization error (thread number overflow)\n");
	if (ret<-4) printf("T2: Initialization error (unknow: %i)\n", ret);
	
	if (ret<0) return 1;
	
	printf("Running...\n");
	
	_sbi_startthread(t1);
	_sbi_startthread(t2);
	
	while (_sbi_running()>0)
	{
		ret = _sbi_stepall(&ctx);
	}
	
	printf("All threads terminated\n");
	
	ret = _sbi_getthreaderror(t1);
	if (ret==1) printf("T1: Program reached end (no exit found)\n");
	if (ret==2) printf("T1: Program exited (no errors)\n");
	if (ret==3) printf("T1: Program exited (wrong instruction code)\n");
	if (ret==4) printf("T1: Program exited (can't understand byte)\n");
	if (ret==5) printf("T1: Program exited (user error)\n");
	
	ret = _sbi_getthreaderror(t2);
	if (ret==1) printf("T2: Program reached end (no exit found)\n");
	if (ret==2) printf("T2: Program exited (no errors)\n");
	if (ret==3) printf("T2: Program exited (wrong instruction code)\n");
	if (ret==4) printf("T2: Program exited (can't understand byte)\n");
	if (ret==5) printf("T2: Program exited (user error)\n");
	
	fclose(f1);
	fclose(f2);
	
	return 0;
}
