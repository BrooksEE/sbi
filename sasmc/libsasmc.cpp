#include "libsasmc.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <map>
#include <sbi_inst.h>
#include "tokenizer.h"


using namespace std;
using std::string;
using std::cout;
using std::endl;

typedef unsigned char byte;

/*
	SBI Format
*/

/*
 Compiler Variables.	
*/
class sasmc_ctx_t {
    public:
    fstream f; // .SBI
    fstream fp; // .SBI.PRG
    int linen;
    byte labelsn;
    int labels[256];
    byte interruptsn;
    int interrupts[256];
    long progln;
    string prgname;
    sasmc_ctx_t() :
        linen(1),
        labelsn(0),
        progln(0),
        interruptsn(0) {
	    for (byte i=0; i<255; i++) labels[i]=0;
    }
};

map<string,int> instructions;


/*
	Enumerations
*/
typedef enum
{
	WRONGNUM,
	WRONGTYPE,
    WRONGCMD
} CERRORTYPE;


/*
	SBI writing routines
*/
void beginsbi(sasmc_ctx_t &ctx)
{
	char buf[2] = { HEADER_0, HEADER_1 };
	ctx.f.write(buf, 2);
}

void endsbi(sasmc_ctx_t& ctx)
{
	char buf[2] = { FOOTER_0, FOOTER_1 };
	ctx.f.write(buf, 2);
}

#define sbiwb(b) _sbiwb(b,ctx)
void _sbiwb(byte b, sasmc_ctx_t& ctx)
{
	char buf[1] = { b };
	ctx.f.write(buf, 1);
}

void sbiwi(int addr, sasmc_ctx_t& ctx)
{
	char buf[2] = {(addr & 0xFF), (addr >> 8)};
	ctx.f.write(buf, 2);
}

#define wb(b) _wb(b,ctx) // easy access to wb for pline
void _wb(byte b, sasmc_ctx_t& ctx)
{
	char buf[1] = { b };
	ctx.fp.write(buf, 1);
	ctx.progln++;
}

void wsbi(sasmc_ctx_t& ctx)
{
	beginsbi(ctx);
	
	int headln = 2 + 1 + 1 + (ctx.labelsn * 2) + 1 + 1 + 1 + (ctx.interruptsn * 2) + 1; // HEADER(2) + LABELSECTION + LABELNUM + LABELDATA + SEPARATOR + INTERRUPTSSECTION + INTERRUPTSN + INTERRUPTSDATA + SEPARATOR
	sbiwb(LABELSECTION);
	sbiwb(ctx.labelsn);
	
	byte n;
	for (n=0; n<ctx.labelsn; n++)
		sbiwi(headln + ctx.labels[n],ctx);
	
	sbiwb(SEPARATOR);
	
	sbiwb(INTERRUPTSECTION);
	sbiwb(ctx.interruptsn);
	
	for (n=0; n<ctx.interruptsn; n++)
		sbiwi(headln + ctx.interrupts[n], ctx);
	
	sbiwb(SEPARATOR);
	
	ctx.fp.close();
	ctx.fp.open(ctx.prgname.c_str(), ios::in | ios::binary);
	
	ctx.fp.seekg(0);
	char buff[ctx.progln];
  ctx.fp.read(buff, ctx.progln);
  
  ctx.f.write(buff, ctx.progln);
	
	endsbi(ctx);
}

/*
	Errors displaying functions
*/
#define cerror(c,t) _cerror(c,t,ctx)
void _cerror(string command, CERRORTYPE type, sasmc_ctx_t &ctx)
{
	switch (type)
	{
		case WRONGNUM:
            printf("%i: (%s) Wrong number of parameters\n", ctx.linen, command.c_str()); 
			break;
		case WRONGTYPE:
			printf("%i: (%s) Wrong type of parameters\n", ctx.linen, command.c_str());
			break;
		default:
			printf("%i: (%s) Syntax error\n", ctx.linen, command.c_str());
			break;
	}
}

// helper compare argt[i] for _varid or _regid
#define VARORREG(i) (argt[i]==_varid||argt[i]==_regid)

/*
	Compilation functions
*/
int pline(string command, int argn, vector<string>& args, sasmc_ctx_t& ctx)
{
	byte argt[8];
	byte argv[8];
	int i=0;
	int p=0;
	for (i=0; i<8; i++)
	{
		argt[i]=_value;
		argv[i]=0;
	}
	for (i=0; i<argn; i++)
	{
		if ((args[i][0]=='_')&&
            ((args[i][1]=='t') ||
             (args[i][1]=='r'))
           )
		{
			string s=args[i].substr(2);
			argt[p]=args[i][1]=='t'?_varid:_regid;
			argv[p]=atoi(s.c_str());
			p++;
		} else 
        {
			argt[p]=_value;
            bool base16 = args[i].size()>2 &&
                          args[i].substr(0,2).compare("0x")==0;
			argv[p]=strtol(args[i].c_str(),NULL, base16?16:10);
			p++;
		}
	}
	if (command.compare("assign")==0)
	{
		if (argn!=2) { cerror(command, WRONGNUM); return 1; }
		if (!VARORREG(0)||(argt[1]!=_value)) { cerror(command, WRONGTYPE); return 1; }
		wb(_istr_assign);
        wb(argt[0]);
		wb(argv[0]);
		wb(argv[1]);
		return 0;
	}
	if (command.compare("move")==0)
	{
		if (argn!=2) { cerror(command, WRONGNUM); return 1; }
		if (!VARORREG(0)) { cerror(command, WRONGTYPE); return 1; }
		wb(_istr_move);
        wb(argt[0]);
		wb(argv[0]);
        wb(argt[1]);
		wb(argv[1]);
		return 0;
	}
	if (command.compare("add")==0 ||
        command.compare("sub")==0 ||
	    command.compare("mul")==0 ||
        command.compare("div")==0 ||
        command.compare("cmp")==0 ||
        command.compare("high")==0 ||
        command.compare("low")==0 ||
        command.compare("lte")==0 ||
        command.compare("gte")==0
       )
    {
		if (argn!=3) { cerror(command, WRONGNUM); return 1; }
		if (!VARORREG(2)) { cerror(command, WRONGTYPE); return 1; }
		wb(instructions[command]);
		wb(argt[0]);
		wb(argv[0]);
		wb(argt[1]);
		wb(argv[1]);
        wb(argt[2]);
		wb(argv[2]);
		return 0;
	}
    if (command.compare("push")==0)
    {
        if (argn!=1) { cerror(command, WRONGNUM); return 1; }
        wb(_istr_push);
        wb(argt[0]);
        wb(argv[0]);
        return 0;
    }
    if (command.compare("pop")==0) 
    {
        if (argn>1) { cerror(command, WRONGNUM); return 1; }
        if (argn==1 && !VARORREG(0)) { cerror(command,WRONGTYPE); return 1; }
        wb(_istr_pop);
        wb(argn);
        if (argn>0) {
            wb(argt[0]);
            wb(argv[0]);
        }
        return 0;
    }
	if (command.compare("incr")==0)
	{
		if (argn!=1) { cerror(command, WRONGNUM); return 1; }
		if (!VARORREG(0)) { cerror(command, WRONGTYPE); return 1; }
		wb(_istr_incr);
        wb(argt[0]);
		wb(argv[0]);
		return 0;
	}
	if (command.compare("decr")==0)
	{
		if (argn!=1) { cerror(command, WRONGNUM); return 1; }
		if (!VARORREG(0)) { cerror(command, WRONGTYPE); return 1; }
		wb(_istr_decr);
        wb(argt[0]);
		wb(argv[0]);
		return 0;
	}
	if (command.compare("inv")==0)
	{
		if (argn!=1) { cerror(command, WRONGNUM); return 1; }
		if (!VARORREG(0)) { cerror(command, WRONGTYPE); return 1; }
		wb(_istr_inv);
        wb(argt[0]);
		wb(argv[0]);
		return 0;
	}
	if (command.compare("tob")==0)
	{
		if (argn!=1) { cerror(command, WRONGNUM); return 1; }
		if (!VARORREG(0)) { cerror(command, WRONGTYPE); return 1; }
		wb(_istr_tob);
        wb(argt[1]);
		wb(argv[0]);
		return 0;
	}
	if (command.compare("label")==0)
	{
		if (argn!=1) { cerror(command, WRONGNUM); return 1; }
		if (argt[0]!=_value) { cerror(command, WRONGTYPE); return 1; }
		ctx.labels[argv[0]] = ctx.progln;
		if ((argv[0]+1) > ctx.labelsn) ctx.labelsn = argv[0]+1;
		return 0;
	}
	if (command.compare("sig")==0)
	{
		if (argn!=1) { cerror(command, WRONGNUM); return 1; }
		if (argt[0]!=_value) { cerror(command, WRONGTYPE); return 1; }
		ctx.interrupts[argv[0]] = ctx.progln;
		if ((argv[0]+1) > ctx.interruptsn) ctx.interruptsn = argv[0]+1;
		return 0;
	}
	if (command.compare("jump")==0)
	{
		if (argn!=2) { cerror(command, WRONGNUM); return 1; }
		if (argt[1]!=_value) { cerror(command, WRONGTYPE); return 1; }
		wb(_istr_jump);
		wb(argt[0]);
		wb(argv[0]);
		wb(argv[1]);
		return 0;
	}
	if (command.compare("cmpjump")==0)
	{
		if (argn!=4) { cerror(command, WRONGNUM); return 1; }
		if (argt[3]!=_value) { cerror(command, WRONGTYPE); return 1; }
		wb(_istr_cmpjump);
		wb(argt[0]);
		wb(argv[0]);
		wb(argt[1]);
		wb(argv[1]);
		wb(argt[2]);
		wb(argv[2]);
		wb(argv[3]);
		return 0;
	}
	if (command.compare("ret")==0)
	{
		if (argn!=0) { cerror(command, WRONGNUM); return 1; }
		wb(_istr_ret);
		return 0;
	}
	if (command.compare("debug")==0)
	{
		if (argn!=1) { cerror(command, WRONGNUM); return 1; }
		wb(_istr_debug);
		wb(argt[0]);
		wb(argv[0]);
		return 0;
	}
	if (command.compare("error")==0)
	{
		if (argn!=1) { cerror(command, WRONGNUM); return 1; }
		wb(_istr_error);
		wb(argt[0]);
		wb(argv[0]);
		return 0;
	}
	if (command.compare("sint")==0)
	{
		if (argn!=1) { cerror(command, WRONGNUM); return 1; }
		wb(_istr_sint);
		wb(argt[0]);
		wb(argv[0]);
		return 0;
	}
	if (command.compare("int")==0)
	{
		wb(_istr_int);
		for (int i=0; i<8; i++)
		{
			wb(argt[i]);
			wb(argv[i]);
		}
		return 0;
	}
    if (command.compare("thread")==0) {
		if (argn!=2) { cerror(command, WRONGNUM); return 1; }
        if (!VARORREG(1)) { cerror(command, WRONGTYPE); return 1; }
        wb(_istr_thread);
        wb(argt[0]);
        wb(argv[0]);
        wb(argt[1]);
        wb(argv[1]);
        return 0;
    }
    if (command.compare("wait")==0) {
        if (argn!=1) { cerror(command, WRONGNUM); return 1; }
        if (!VARORREG(0)) { cerror(command, WRONGTYPE); return 1; }
        wb(_istr_wait);
        wb(argt[0]);
        wb(argv[0]);
        return 0;
    }
	if (command.compare("exit")==0)
	{
		if (argn!=0) { cerror(command, WRONGNUM); return 1; }
		wb(_istr_exit);
		return 0;
	}

    // TODO - make a better tokenizer
    if (command.compare("\r")==0 ||
        command.compare("")==0
       )
        return 0;
    printf ( "Unhandled token: (%s)\n" , command.c_str() );
    return 1;
}

int sasmc (
    const std::string &src,
    const std::string &dst,
    bool silent,
    bool clean ) {
    
    if (!instructions.size()) {
        // todo maybe better way to do this
        instructions["add"] = _istr_add;
        instructions["assign"] = _istr_assign;
   	    instructions["move"] = _istr_move;
	    instructions["add"] = _istr_add;
	    instructions["sub"] = _istr_sub;
	    instructions["mul"] = _istr_mul;
	    instructions["div"] = _istr_div;
        instructions["push"] = _istr_push;
        instructions["pop"] = _istr_pop;
	    instructions["incr"] = _istr_incr;
	    instructions["decr"] = _istr_decr;
	    instructions["inv"] = _istr_inv;
	    instructions["tob"] = _istr_tob;
	    instructions["cmp"] = _istr_cmp;
	    instructions["high"] = _istr_high;
	    instructions["low"] = _istr_low;
        instructions["lte"] = _istr_lte;
        instructions["gte"] = _istr_gte;
	    instructions["jump"] = _istr_jump;
	    instructions["cmpjump"] = _istr_cmpjump;
	    instructions["ret"] = _istr_ret;
	    instructions["debug"] = _istr_debug;
	    instructions["error"] = _istr_error;
	    instructions["sint"] = _istr_sint;
	    instructions["int"] = _istr_int;
        instructions["thread"] = _istr_thread;
        instructions["wait "] = _istr_wait;
	    instructions["exit"] = _istr_exit;
    }

    sasmc_ctx_t ctx;
    

	fstream file(src.c_str(), ios::in);

 	ctx.prgname = dst + ".prg";

 	ctx.f.open(dst.c_str(), ios::out | ios::binary);
 	ctx.fp.open(ctx.prgname.c_str(), ios::out | ios::binary);

	if (!file) { printf("Can't open .SASM file for reading!\n"); return 1; }
	if (!ctx.f) { printf("Can't open .SBI file for writing!\n"); return 1; }
	if (!ctx.fp) { printf("Can't open .SBI.PRG file for writing!\n"); return 1; }
  
  
	if (silent==false) printf("SASMC %s :: Assembling %s :: Please wait... ", VERSION_STR, src.c_str());
  
	string line;
	
	while(std::getline(file, line))
	{
		Tokenizer str;
		string token;
		string command;
		vector<string> tokens(16);
		int cnt = 0;
		int ret;
	    	
		if (line.find(';')!=-1) line = line.substr(0, line.find(';'));
		if (line!="")
		{
			str.set(line, " \t");
    
			while((token = str.next()) != "")
			{
				if (cnt==0) command = token; else tokens[cnt-1] = token;
				cnt++;
			}
	    		
			int nt = cnt-1;
			ret = pline(command.c_str(), nt, tokens, ctx);
			if (ret>0) { ctx.f.close(); ctx.fp.close(); file.close(); remove(dst.c_str()); remove(ctx.prgname.c_str()); return 1; }
		}
		
		ctx.linen++;
	}
	
	wsbi(ctx);
	
	if (silent==false) printf("Done!\n");
	
	ctx.f.close();
	ctx.fp.close();
	file.close();
	
	if (clean==true)
	{
		if (silent==false) printf("SASMC %s :: Cleaning %s :: Please wait... ", VERSION_STR, ctx.prgname.c_str());
		remove(ctx.prgname.c_str());
		if (silent==false) printf("Done!\n");
	}


    return 0;
}
 
