#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sbi_inst.h>
#include "tokenizer.h"

#define VERSION_STR "0.4"

using namespace std;
using std::string;
using std::cout;
using std::endl;

typedef unsigned char byte;

/*
	SBI Format
*/

/*
	Program global variables
*/
fstream f; // .SBI
fstream fp; // .SBI.PRG
int linen = 1;
byte labelsn = 0;
int labels[256];
byte interruptsn = 0;
int interrupts[256];
long progln=0;
char* inname;
char* outname;
char* prgname;

/*
	Enumerations
*/
typedef enum
{
	WRONGNUM,
	WRONGTYPE
} CERRORTYPE;

/*
	Command-line arguments parsing functions
*/
char* getCmdOption(char ** begin, char ** end, const std::string & option)
{
    char ** itr = std::find(begin, end, option);
    if (itr != end && ++itr != end)
    {
        return *itr;
    }
    return 0;
}

bool cmdOptionExists(char** begin, char** end, const std::string& option)
{
    return std::find(begin, end, option) != end;
}

/*
	SBI writing routines
*/
void beginsbi(void)
{
	char buf[2] = { HEADER_0, HEADER_1 };
	f.write(buf, 2);
}

void endsbi(void)
{
	char buf[2] = { FOOTER_0, FOOTER_1 };
	f.write(buf, 2);
}

void sbiwb(byte b)
{
	char buf[1] = { b };
	f.write(buf, 1);
}

void sbiwi(int addr)
{
	char buf[2] = {(addr & 0xFF), (addr >> 8)};
	f.write(buf, 2);
}

void wb(byte b)
{
	char buf[1] = { b };
	fp.write(buf, 1);
	progln++;
}

void wsbi(void)
{
	beginsbi();
	
	int headln = 2 + 1 + 1 + (labelsn * 2) + 1 + 1 + 1 + (interruptsn * 2) + 1; // HEADER(2) + LABELSECTION + LABELNUM + LABELDATA + SEPARATOR + INTERRUPTSSECTION + INTERRUPTSN + INTERRUPTSDATA + SEPARATOR
	sbiwb(LABELSECTION);
	sbiwb(labelsn);
	
	byte n;
	for (n=0; n<labelsn; n++)
		sbiwi(headln + labels[n]);
	
	sbiwb(SEPARATOR);
	
	sbiwb(INTERRUPTSECTION);
	sbiwb(interruptsn);
	
	for (n=0; n<interruptsn; n++)
		sbiwi(headln + interrupts[n]);
	
	sbiwb(SEPARATOR);
	
	fp.close();
	fp.open(prgname, ios::in | ios::binary);
	
	fp.seekg(0);
	char buff[progln];
  fp.read(buff, progln);
  
  f.write(buff, progln);
	
	endsbi();
}

/*
	Errors displaying functions
*/
void cerror(string command, CERRORTYPE type)
{
	switch (type)
	{
		case WRONGNUM:
            printf("%i: (%s) Wrong number of parameters\n", linen, command.c_str()); 
			break;
		case WRONGTYPE:
			printf("%i: (%s) Wrong type of parameters\n", linen, command.c_str());
			break;
		default:
			printf("%i: (%s) Syntax error\n", linen, command.c_str());
			break;
	}
}

/*
	Compilation functions
*/
int pline(string command, int argn, vector<string>& args)
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
		if ((args[i][0]=='_')&&(args[i][1]=='t'))
		{
			string s=args[i].substr(2);
			argt[p]=_varid;
			argv[p]=atoi(s.c_str());
			p++;
		} else {
			argt[p]=_value;
			argv[p]=atoi(args[i].c_str());
			p++;
		}
	}
	if (command.compare("assign")==0)
	{
		if (argn!=2) { cerror(command, WRONGNUM); return 1; }
		if ((argt[0]!=_varid)||(argt[1]!=_value)) { cerror(command, WRONGTYPE); return 1; }
		wb(_istr_assign);
		wb(argv[0]);
		wb(argv[1]);
		return 0;
	}
	if (command.compare("move")==0)
	{
		if (argn!=2) { cerror(command, WRONGNUM); return 1; }
		if ((argt[0]!=_varid)||(argt[1]!=_varid)) { cerror(command, WRONGTYPE); return 1; }
		wb(_istr_move);
		wb(argv[0]);
		wb(argv[1]);
		return 0;
	}
	if (command.compare("add")==0)
	{
		if (argn!=3) { cerror(command, WRONGNUM); return 1; }
		if (argt[2]!=_varid) { cerror(command, WRONGTYPE); return 1; }
		wb(_istr_add);
		wb(argt[0]);
		wb(argv[0]);
		wb(argt[1]);
		wb(argv[1]);
		wb(argv[2]);
		return 0;
	}
	if (command.compare("sub")==0)
	{
		if (argn!=3) { cerror(command, WRONGNUM); return 1; }
		if (argt[2]!=_varid) { cerror(command, WRONGTYPE); return 1; }
		wb(_istr_sub);
		wb(argt[0]);
		wb(argv[0]);
		wb(argt[1]);
		wb(argv[1]);
		wb(argv[2]);
		return 0;
	}
	if (command.compare("mul")==0)
	{
		if (argn!=3) { cerror(command, WRONGNUM); return 1; }
		if (argt[2]!=_varid) { cerror(command, WRONGTYPE); return 1; }
		wb(_istr_mul);
		wb(argt[0]);
		wb(argv[0]);
		wb(argt[1]);
		wb(argv[1]);
		wb(argv[2]);
		return 0;
	}
	if (command.compare("div")==0)
	{
		if (argn!=3) { cerror(command, WRONGNUM); return 1; }
		if (argt[2]!=_varid) { cerror(command, WRONGTYPE); return 1; }
		wb(_istr_div);
		wb(argt[0]);
		wb(argv[0]);
		wb(argt[1]);
		wb(argv[1]);
		wb(argv[2]);
		return 0;
	}
	if (command.compare("incr")==0)
	{
		if (argn!=1) { cerror(command, WRONGNUM); return 1; }
		if (argt[0]!=_varid) { cerror(command, WRONGTYPE); return 1; }
		wb(_istr_incr);
		wb(argv[0]);
		return 0;
	}
	if (command.compare("decr")==0)
	{
		if (argn!=1) { cerror(command, WRONGNUM); return 1; }
		if (argt[0]!=_varid) { cerror(command, WRONGTYPE); return 1; }
		wb(_istr_decr);
		wb(argv[0]);
		return 0;
	}
	if (command.compare("inv")==0)
	{
		if (argn!=1) { cerror(command, WRONGNUM); return 1; }
		if (argt[0]!=_varid) { cerror(command, WRONGTYPE); return 1; }
		wb(_istr_inv);
		wb(argv[0]);
		return 0;
	}
	if (command.compare("tob")==0)
	{
		if (argn!=1) { cerror(command, WRONGNUM); return 1; }
		if (argt[0]!=_varid) { cerror(command, WRONGTYPE); return 1; }
		wb(_istr_tob);
		wb(argv[0]);
		return 0;
	}
	if (command.compare("cmp")==0)
	{
		if (argn!=3) { cerror(command, WRONGNUM); return 1; }
		if (argt[2]!=_varid) { cerror(command, WRONGTYPE); return 1; }
		wb(_istr_cmp);
		wb(argt[0]);
		wb(argv[0]);
		wb(argt[1]);
		wb(argv[1]);
		wb(argv[2]);
		return 0;
	}
	if (command.compare("high")==0)
	{
		if (argn!=3) { cerror(command, WRONGNUM); return 1; }
		if (argt[2]!=_varid) { cerror(command, WRONGTYPE); return 1; }
		wb(_istr_high);
		wb(argt[0]);
		wb(argv[0]);
		wb(argt[1]);
		wb(argv[1]);
		wb(argv[2]);
		return 0;
	}
	if (command.compare("low")==0)
	{
		if (argn!=3) { cerror(command, WRONGNUM); return 1; }
		if (argt[2]!=_varid) { cerror(command, WRONGTYPE); return 1; }
		wb(_istr_low);
		wb(argt[0]);
		wb(argv[0]);
		wb(argt[1]);
		wb(argv[1]);
		wb(argv[2]);
		return 0;
	}
	if (command.compare("label")==0)
	{
		if (argn!=1) { cerror(command, WRONGNUM); return 1; }
		if (argt[0]!=_value) { cerror(command, WRONGTYPE); return 1; }
		labels[argv[0]] = progln;
		if ((argv[0]+1) > labelsn) labelsn = argv[0]+1;
		return 0;
	}
	if (command.compare("sig")==0)
	{
		if (argn!=1) { cerror(command, WRONGNUM); return 1; }
		if (argt[0]!=_value) { cerror(command, WRONGTYPE); return 1; }
		interrupts[argv[0]] = progln;
		if ((argv[0]+1) > interruptsn) interruptsn = argv[0]+1;
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
        if (argt[1]!=_varid) { cerror(command, WRONGTYPE); return 1; }
        wb(_istr_thread);
        wb(argt[0]);
        wb(argv[0]);
        wb(argv[1]);
        return 0;
    }
    if (command.compare("wait")==0) {
        if (argn!=1) { cerror(command, WRONGNUM); return 1; }
        if (argt[0] != _varid) { cerror(command, WRONGTYPE); return 1; }
        wb(_istr_wait);
        wb(argv[0]);
        return 0;
    }
	if (command.compare("exit")==0)
	{
		if (argn!=0) { cerror(command, WRONGNUM); return 1; }
		wb(_istr_exit);
		return 0;
	}
}

/*
	Main
*/
int main (int argc, char** argv)
{
	bool silent=false;
	bool clean=false;
	
	if(cmdOptionExists(argv, argv+argc, "-i")==false)
	{
		printf("SASM Compiler\n ver %s by Gi@cky98\n\n", VERSION_STR);
		printf("Usage: %s -i input.sasm [-o output.sbi] [-s] [-cl]\n\n", (char*)argv[0]);
		printf("-i input.sasm\t\tSASM assembly file to compile\n");
		printf("-o output.sbi\t\tSBI output filename\n");
		printf("-s\t\t\tSilent mode\n");
		printf("-cl\t\t\tClean non-SBI files used during compilation\n");
		return 1;
	}

	for (byte i=0; i<255; i++) labels[i]=0;
	
	inname = getCmdOption(argv, argv + argc, "-i");
	fstream file(inname, ios::in);

	if(cmdOptionExists(argv, argv + argc, "-o"))
		outname = getCmdOption(argv, argv + argc, "-o");
	else
		outname = (char*)"out.sbi";
	
	char buf[128];
	strcpy(buf, outname);
	strcat(buf, ".prg");
 	prgname = (char*)buf;
	
 	f.open(outname, ios::out | ios::binary);
 	fp.open(prgname, ios::out | ios::binary);

	if (!file) { printf("Can't open .SASM file for reading!\n"); return 1; }
	if (!f) { printf("Can't open .SBI file for writing!\n"); return 1; }
	if (!fp) { printf("Can't open .SBI.PRG file for writing!\n"); return 1; }
  
	if(cmdOptionExists(argv, argv + argc, "-s")) silent = true;
	if(cmdOptionExists(argv, argv + argc, "-cl")) clean = true;
  
	if (silent==false) printf("SASMC %s :: Assembling %s :: Please wait... ", VERSION_STR, inname);
  
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
			ret = pline(command.c_str(), nt, tokens);
			if (ret>0) { f.close(); fp.close(); file.close(); remove(outname); remove(prgname); return 1; }
		}
		
		linen++;
	}
	
	wsbi();
	
	if (silent==false) printf("Done!\n");
	
	f.close();
	fp.close();
	file.close();
	
	if (clean==true)
	{
		if (silent==false) printf("SASMC %s :: Cleaning %s :: Please wait... ", VERSION_STR, prgname);
		remove(prgname);
		if (silent==false) printf("Done!\n");
	}
	
	return 0;
}
