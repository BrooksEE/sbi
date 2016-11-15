#include "libsasmc.h"

#include <cstdlib>
#include <cstdio>
#ifdef _MSC_VER
typedef unsigned __int32 uint32_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int8 uint8_t;
#else
#include <stdint.h>
#endif
#include <cstring>
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
    uint8_t labelsn;
    int labels[256];
    uint8_t interruptsn;
    int interrupts[256];
    vector<string> print_strings; // unique strings
    map<int,int> print_strloc_map; // location of print statements.
    long progln;
    string prgname;
    bool verbose;
    bool silent;
    sasmc_ctx_t() :
        linen(1),
        labelsn(0),
        progln(0),
        interruptsn(0) {
        memset(labels,0,sizeof(labels));
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
	uint8_t buf[2] = { HEADER_0, HEADER_1 };
	ctx.f.write((char*)buf, 2);
}

void endsbi(sasmc_ctx_t& ctx)
{
	uint8_t buf[2] = { FOOTER_0, FOOTER_1 };
	ctx.f.write((char*)buf, 2);
}

#define sbiwb(b) _sbiwb(b,ctx)
void _sbiwb(uint8_t b, sasmc_ctx_t& ctx)
{
	uint8_t buf[1] = { b };
	ctx.f.write((char*)buf, 1);
}

void sbiwi(int addr, sasmc_ctx_t& ctx)
{
	uint8_t buf[2] = {(addr & 0xFF), (addr >> 8)};
	ctx.f.write((char*)buf, 2);
}

#define wb(b) _wb(b,ctx) // easy access to wb for pline
void _wb(uint8_t b, sasmc_ctx_t& ctx)
{
	uint8_t buf[1] = { b };
	ctx.fp.write((char*)buf, 1);
	ctx.progln++;
}

void wsbi(sasmc_ctx_t& ctx)
{
	beginsbi(ctx);
	
	int headln = 2 + 1 + 1 + (ctx.labelsn * 2) + 1 + 1 + 1 + (ctx.interruptsn * 2) + 1; // HEADER(2) + LABELSECTION + LABELNUM + LABELDATA + SEPARATOR + INTERRUPTSSECTION + INTERRUPTSN + INTERRUPTSDATA + SEPARATOR
	sbiwb(LABELSECTION);
	sbiwb(ctx.labelsn);
	
	uint8_t n;
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
	char *buff = new char[ctx.progln];
    ctx.fp.read(buff, ctx.progln);

    map<int,int> str_locs;
    int progln=headln + ctx.progln+2; // for the footer 
    for (int i=0;i<ctx.print_strings.size();++i) {
        str_locs[i] = progln; 
        progln += ctx.print_strings[i].size() + 1;
    }
    // now replace print statement locations with string positions.
    for (map<int,int>::iterator itr = ctx.print_strloc_map.begin();
         itr != ctx.print_strloc_map.end();
         ++itr ) {
         int pos = itr->first;
         int strIdx = itr->second;
         int strPos = str_locs[strIdx];
         buff[pos] = strPos & 0xff; 
         buff[pos+1] = strPos >> 8;
    }
  
    ctx.f.write(buff, ctx.progln);
	endsbi(ctx);

    // now write the strings
    for (int i=0;i<ctx.print_strings.size();++i) {
        ctx.f.write( ctx.print_strings[i].c_str(), ctx.print_strings[i].size() );
        ctx.f.put ( '\0' );
    }
	
	delete [] buff;
	
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
#define VARORREG(i) ((argv[i].type&0xC0)==0x80)
#define CONST(i) ((argv[i].type&0xC0)==0xC0)

#define WV32(t,val) do {\
      wb(val & 0xFF); \
      if (t != _value16 && t != _value32) break; \
      wb((val >>8) & 0xFF); \
      if (t == _value16) break; \
      wb((val >>16) & 0xFF); \
      wb((val >>24) & 0xFF); \
    } while (0) 

#define WVAL(i) do {\
            wb(argv[i].type);\
            switch(argv[i].type) {\
               case _stackid:\
               case _varid:\
               case _regid: \
                  wb(argv[i].iVal); \
                  break;\
               case _stackpid: \
                  break;\
               case _stackvid: \
                  wb(argv[i].vType); \
                  WV32(argv[i].vType, argv[i].vVal); \
                  break; \
               case _value8:\
               case _value16: \
               case _value32: \
                  WV32(argv[i].type, argv[i].iVal); \
                  break; \
            }\
       } while(0)


struct argv_t {
  uint8_t type;
  uint32_t iVal;
  uint8_t vType; // for stackvid
  uint32_t vVal;
};

argv_t parseInt(string token, unsigned max, bool base16ok, bool &ok) {
    bool base16 = token.size()>2 &&
                  token.substr(0,2).compare("0x")==0;
    argv_t v;
    if (base16 && !base16ok) {ok=false; return v;}

    for (int i=base16?2:0; i<token.size(); ++i ) {
        char d=token[i]; 
        if (d >= '0' && d <= '9') continue;
        if (base16 && d>='a' && d<='f') continue;
        if (base16 && d>='A' && d<='F') continue;
        ok=false;
        return v;
    }
 
    v.iVal = strtoul(token.c_str(),NULL, base16?16:10);
    if (v.iVal<=0xff) v.type = _value8;
    else if (v.iVal<=0xffff) v.type = _value16;
    else v.type = _value32;

    if (v.iVal > max) ok=false;

    return v;
}

argv_t parseNonString(string token, bool &ok) {
    argv_t v;
    ok=true; 
    switch (token[0] ) {
        case '_':
            switch (token[1]) {
                case 't': 
                    v.type=_varid;
                    {
                      argv_t tmp=parseInt(token.substr(2), 63, false, ok); // TODO magic hard coded numbers 
                                                                              // because sbi.h isn't included? 
                                                                              // should it be?
                                                                              // how to enforce the same compile limits.
                      v.iVal=tmp.iVal;
                    }
                    break;
                case 'r':
                    v.type=_regid;
                    {
                        argv_t tmp=parseInt(token.substr(2), 15, false, ok );
                        v.iVal=tmp.iVal;
                    }
                    break;
                case 's': 
                    switch (token[2]) {
                        case 'p': 
                            v.type=_stackpid;
                            break;
                        case '+': 
                            v.type=_stackvid;
                            {
                                argv_t tmp=parseNonString(token.substr(3), ok);
                                v.vType = tmp.type;
                                v.vVal = tmp.iVal;
                                break;
                            }
                        default:
                            v.type=_stackid;
                            {
                                argv_t tmp=parseInt(token.substr(2), 63, false, ok);
                                v.iVal=tmp.iVal;
                            }
                            break;
                    }
            }
            return v;
            break;
        default:
            return parseInt(token, 0xffffffffu, true, ok);
    }
        
}    

/*
	Compilation functions
*/
int pline(string command, vector<string>& args, sasmc_ctx_t& ctx)
{
    int argn=args.size();
	vector<argv_t> argv(argn);

    // special handle ':' on label
    if (command == "label") {
        if (argn == 2 && args[1] != ":") {
            cerror ( command, WRONGTYPE );
            return 1;
        } else if (argn == 2) 
            --argn; // ignore the ':'
    }
	
	if (ctx.verbose) {
	   printf ( "Command: %s Args: " , command.c_str() );
	   for (vector<string>::iterator itr = args.begin();
	                                 itr != args.end();
	                                 ++itr ) {
          printf ( " %s", itr->c_str() );
       }
       printf ( "\n" );
	}

    if (command == "print") {
        if (argn != 1) { cerror(command,WRONGNUM); return 1; }
        string val=args.front();
        if (val[0] != '"' || val[val.size()-1] != '"') { cerror(command,WRONGTYPE); return 1; }
        // remove quotes
        val = val.substr(1, val.size()-2);
        int psLoc;
        for (psLoc = 0; psLoc < ctx.print_strings.size(); ++psLoc )
            if (ctx.print_strings[psLoc] == val)
                break;
        if (psLoc == ctx.print_strings.size())
            ctx.print_strings.push_back(val);
        wb(_istr_print);
        ctx.print_strloc_map[ctx.progln] = psLoc;
        wb(0xDD); // tmp replace w/ str loc later on.
        wb(0xEE);
        return 0;
    }

	for (int i=0; i<argn; i++)
	{
        bool ok;
	    argv[i] = parseNonString(args[i], ok);
        if (!ok) {
            cerror ( command, WRONGTYPE ); return 1; 
        }
	}
	
	if (command.compare("assign")==0)
	{
		if (argn!=2) { cerror(command, WRONGNUM); return 1; }
		if (!VARORREG(0) || !CONST(1)) { cerror(command, WRONGTYPE); return 1; }
		if (!ctx.silent) printf ( "assign instruction is depricated.  Use move.\n" );
		wb(_istr_assign);
        WVAL(0);
        WVAL(1);
		return 0;
	}
	if (command.compare("move")==0)
	{
		if (argn!=2) { cerror(command, WRONGNUM); return 1; }
		if (!VARORREG(0)) { cerror(command, WRONGTYPE); return 1; }
		wb(_istr_move);
        WVAL(0);
        WVAL(1);
		return 0;
	}
	if (command.compare("add")==0 ||
        command.compare("sub")==0 ||
	    command.compare("mul")==0 ||
        command.compare("div")==0 ||
        command.compare("mod")==0 ||
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
        WVAL(0);
        WVAL(1);
        WVAL(2);
		return 0;
	}
    if (command.compare("push")==0)
    {
        if (argn!=1) { cerror(command, WRONGNUM); return 1; }
        wb(_istr_push);
        WVAL(0);
        return 0;
    }
    if (command.compare("pop")==0) 
    {
        if (argn>1) { cerror(command, WRONGNUM); return 1; }
        if (argn==1 && !VARORREG(0)) { cerror(command,WRONGTYPE); return 1; }
        wb(_istr_pop);
        wb(argn);
        if (argn>0) {
            WVAL(0);
        }
        return 0;
    }
	if (command.compare("incr")==0 || 
        command.compare("decr")==0 || 
        command.compare("inv")==0  ||
        command.compare("tob")==0
       )
	{
		if (argn!=1) { cerror(command, WRONGNUM); return 1; }
		if (!VARORREG(0)) { cerror(command, WRONGTYPE); return 1; }
		wb(instructions[command]);
		WVAL(0);
		return 0;
	}
	if (command.compare("label")==0)
	{
		if (argn!=1) { cerror(command, WRONGNUM); return 1; }
		if (!CONST(0)) { cerror(command, WRONGTYPE); return 1; }
		ctx.labels[argv[0].iVal] = ctx.progln;
		if ((argv[0].iVal+1) > ctx.labelsn) ctx.labelsn = argv[0].iVal+1;
		return 0;
	}
	if (command.compare("sig")==0)
	{
		if (argn!=1) { cerror(command, WRONGNUM); return 1; }
		if (!CONST(0)) { cerror(command, WRONGTYPE); return 1; }
		ctx.interrupts[argv[0].iVal] = ctx.progln;
		if ((argv[0].iVal+1) > ctx.interruptsn) ctx.interruptsn = argv[0].iVal+1;
		return 0;
	}
	if (command.compare("jump")==0)
	{
		if (argn!=2) { cerror(command, WRONGNUM); return 1; }
		if (!CONST(1)) { cerror(command, WRONGTYPE); return 1; }
		wb(_istr_jump);
        WVAL(0);
		wb(argv[1].iVal); // note leaving as one byte since it's really just a 1 or 0
		return 0;
	}
	if (command.compare("cmpjump")==0)
	{
		if (argn!=4) { cerror(command, WRONGNUM); return 1; }
		if (!CONST(3)) { cerror(command, WRONGTYPE); return 1; }
		wb(_istr_cmpjump);
        WVAL(0);
        WVAL(1);
        WVAL(2);
		wb(argv[3].iVal);
		return 0;
	}
	if (command.compare("ret")==0)
	{
		if (argn!=0) { cerror(command, WRONGNUM); return 1; }
		wb(_istr_ret);
		return 0;
	}
	if (command.compare("debug")==0 ||
	    command.compare("error")==0 ||
        command.compare("sint")==0 ||
        command.compare("printd")== 0
       )
    {
		if (argn!=1) { cerror(command, WRONGNUM); return 1; }
		wb(instructions[command]);
        WVAL(0);
		return 0;
	}
	if (command.compare("int")==0
       )
	{
		wb(_istr_int);
        wb(argn);
        for (int i=0; i<argn; i++)
		{
            WVAL(i);
		}
		return 0;
	}
    if (command.compare("intr")==0)
    {
       if (argn<1) { cerror(command, WRONGNUM); return 1; }
       if (!VARORREG(0)) { cerror(command, WRONGTYPE); return 1; }
       wb(_istr_intr);
       WVAL(0);
       wb(argn-1);
       for (int i=1;i<argn;++i) WVAL(i);
       return 0;
    }
    if (command.compare("thread")==0) {
		if (argn!=2) { cerror(command, WRONGNUM); return 1; }
        if (!VARORREG(1)) { cerror(command, WRONGTYPE); return 1; }
        wb(_istr_thread);
        WVAL(0);
        WVAL(1);
        return 0;
    }
    if (command.compare("wait")==0) {
        if (argn!=1) { cerror(command, WRONGNUM); return 1; }
        if (!VARORREG(0)) { cerror(command, WRONGTYPE); return 1; }
        wb(_istr_wait);
        WVAL(0);
        return 0;
    }
    if (command.compare("alive")==0) {
        if (argn!=2) { cerror(command, WRONGNUM); return 1; }
        if (!VARORREG(0) || !VARORREG(1)) { cerror(command, WRONGTYPE); return 1; }
        wb(_istr_alive);
        WVAL(0);
        WVAL(1);
        return 0;
    }
    if (command.compare("stop")==0) {
        if (argn!=1) { cerror(command,WRONGNUM); return 1; }
        if (!VARORREG(0)) { cerror(command,WRONGTYPE); return 1; }
        wb(_istr_stop);
        WVAL(0);
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
    bool clean,
    bool verbose ) {
    
    if (!instructions.size()) {
        // todo maybe better way to do this
        instructions["add"] = _istr_add;
        instructions["assign"] = _istr_assign;
   	    instructions["move"] = _istr_move;
	    instructions["add"] = _istr_add;
	    instructions["sub"] = _istr_sub;
	    instructions["mul"] = _istr_mul;
	    instructions["div"] = _istr_div;
        instructions["mod"] = _istr_mod;
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
        instructions["print"] = _istr_print;
        instructions["printd"] = _istr_printd;
	    instructions["sint"] = _istr_sint;
	    instructions["int"] = _istr_int;
        instructions["intr"] = _istr_intr;
        instructions["thread"] = _istr_thread;
        instructions["wait "] = _istr_wait;
        instructions["alive"] = _istr_alive;
        instructions["stop"] = _istr_stop;
	    instructions["exit"] = _istr_exit;
    }

    sasmc_ctx_t ctx;
    

	fstream file(src.c_str(), ios::in);

 	ctx.prgname = dst + ".prg";
 	ctx.verbose = verbose;
 	ctx.silent = silent;

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
		vector<string> tokens;
		int ret;
	    	
		if (line!="")
		{
			str.set(line, DEFAULT_DELIMITER + ";:");
            tokens.clear();
    
			while((token = str.next(true)) != "")
			{
                if (token == ";") break;
                if (token.size() > 1 || DEFAULT_DELIMITER.find(token[0]) == string::npos )
				    tokens.push_back(token);
			}
            if (tokens.empty()) continue;
	        command = tokens.front();
            tokens.erase(tokens.begin());
			ret = pline(command.c_str(), tokens, ctx);
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
 
