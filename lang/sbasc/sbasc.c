/*
	SBASC COMPILER
	
	Sbi BASic Compiler
	Compiles SBAS program files into SASM assembly files
*/

#include <stdio.h>
#include <stdlib.h>

#include <string.h>

#include <unistd.h>

#define C(n) printf("Checkpoint: %i\n", n);
#define T(s) printf("Debug:      %s\n", s);
#define H(c) printf("Debug: CH:  %c\n", c);
#define P()  getc(stdin);

#define MAX_DIGITS		5

/*
	Basic language lookup table
*/
char* baslookup[] = {
	"PRINT",
	"IF",
	"THEN",
	"ELSE",
	"END",
	"FOR",
	"TO",
	"STEP",
	"NEXT",
	"WHILE",
	"WEND",
	"REPEAT",
	"UNITL",
	"DO",
	"LOOP",
	"GOTO",
	"GOSUB",
	"ON",
	"LET",
	"REM"
};

typedef enum {
	PRINT,
	IF,
	THEN,
	ELSE,
	END,
	FOR,
	TO,
	STEP,
	NEXT,
	WHILE,
	WEND,
	REPEAT,
	UNTIL,
	DO,
	LOOP,
	GOTO,
	GOSUB,
	ON,
	LET,
	REM,
	UNKNOWN
} KEYWORD;

char oplookup[] = {
	'+',
	'-',
	'*',
	'/',
	'=',
	'>',
	'<',
	':'
};

typedef enum
{
	ADD,
	SUB,
	MUL,
	DIV,
	EQUAL,
	HIGH,
	LOW,
	DP
} OPERATOR;

/*
	Datatypes
*/
typedef unsigned int NUMBER;

typedef char* VARNAME;

typedef enum
{
	TKEYWORD,
	TNUMBER,
	TOPERATOR,
	TVARNAME,
	TUNKNOWN
} TOKENTYPE;

typedef struct
{
	TOKENTYPE t;
	KEYWORD keyword;
	NUMBER number;
	OPERATOR operator;
	VARNAME varname;
} TOKEN;

typedef struct
{
	NUMBER tokensnum;
	TOKEN* tokens;
	char* original;
	NUMBER originaln;
	NUMBER label;
	NUMBER oi;
} LINE;

/*
	Prototypes
*/
KEYWORD		checkKeyword(char* s);				// Check what keyword is the specified string
OPERATOR	checkOperator(char* s);				// Check what operator is the specified string
LINE*		parseLine(char* s);				// Parses a SBAS line (from the stream <f>) into a <LINE> struct
int		runParser(FILE* f);				// Parses all the lines of the stream <f>
int	lineToASM(FILE* f, LINE* al, int linen);		// Parses a <struct LINE> and writes the SASM code to the stream
int		compileAll(FILE* i, FILE* o);			// Compiles the SBAS file <i> into a SASM file <o>

/*
	Global variables
*/
unsigned int line = 1;
unsigned int linesn;
LINE* lines;

VARNAME* progVars;
int progVarsN = 0;

NUMBER* progLabels;
int progLabelsN = 0;

char** userLabels;
int userLabelsN;

NUMBER maxLabel = 0;

int commentCode = 0;

/*
	Main
*/
int main(int argc, char** argv)
{
	// Streams
	FILE* sbasf; // .SBAS file stream (input)
	FILE* sasmf; // .SASM file stream (output)
	
	printf("SBAS Compiler\n");
	
	commentCode = 1;
	printf("Compiling test.bas...\n");
	sbasf = fopen("test.bas", "r");
	sasmf = fopen("test.sasm", "w");
	int r = compileAll(sbasf, sasmf);
	fclose(sbasf);
	fclose(sasmf);
	printf("Ret: %i\n", r);
	
	return 0;
}

/*
	Functions
*/
void addVariable(VARNAME name)
{
	progVars = (VARNAME*) realloc(progVars, (progVarsN + 1) * sizeof(VARNAME));
	progVars[progVarsN] = (VARNAME) malloc(strlen(name));
	strcpy((char*)progVars[progVarsN], (char*)name);
	progVarsN++;
}

int existVariable(VARNAME name)
{
	int i = progVarsN;
	while (i--)
		if (strcmp((char*)progVars[i], (char*)name) == 0) return 1;
	return 0;
}

int getVarNum(VARNAME name)
{
	int i = progVarsN;
	while (i--)
		if (strcmp((char*)progVars[i], (char*)name) == 0) return i;
	return -1;
}

int getTempVar(void)
{
	if (!existVariable((VARNAME)"temp")) addVariable((VARNAME)"temp");
	return getVarNum((VARNAME)"temp");
}

void addLabel(NUMBER line)
{
	progLabels = (NUMBER*) realloc(progLabels, (progLabelsN + 1) * sizeof(NUMBER));
	progLabels[progLabelsN] = line;
	progLabelsN++;
}

int existLabel(NUMBER line)
{
	int i = progLabelsN;
	while (i--)
		if (progLabels[i] == line) return 1;
	return 0;
}

int getLabelNum(NUMBER line)
{
	int i = progLabelsN;
	while (i--)
		if (progLabels[i] == line) return (linesn - 1) + maxLabel + 1 + i;
	return -1;
}

void addUserLabel(char* name)
{
	userLabels = (char**) realloc(userLabels, (userLabelsN + 1) * sizeof(char*));
	userLabels[userLabelsN] = (char*) malloc(strlen(name));
	strcpy(userLabels[userLabelsN], name);
	userLabelsN++;
}

int existUserLabel(char* name)
{
	int i = userLabelsN;
	while (i--)
		if (strcmp(userLabels[i], name) == 0) return 1;
	return 0;
}

int getUserLabelNum(char* name)
{
	int i = userLabelsN;
	while (i--)
		if (strcmp(userLabels[i], name) == 0) return i;
	return -1;
}

KEYWORD checkKeyword(char* s)
{
	int i; for (i = 0; i < sizeof(baslookup) / sizeof(char*); i++)
		if (strcmp(s, baslookup[i]) == 0) return i; // If found return its index,
	
	return UNKNOWN; // else return 'unknown' keyword
}

OPERATOR checkOperator(char* s)
{
	int i; for (i = 0; i < sizeof(oplookup) / sizeof(char); i++)
		if (*s == oplookup[i]) return i; // If found return its index
	
	return -1; // else return -1 (error)
}

int isDigit(char ch)
{
	return ((ch >= '0') && (ch <= '9'));
}

LINE* parseLine(char* s)
{
	const char se = '\0';
	
	static LINE* l;						// Line output structure (pointer)
	l = (LINE*) malloc(sizeof(LINE));
	l->tokens = (TOKEN*) malloc(sizeof(TOKEN) * 1);
	l->original = (char*) malloc(strlen(s) - 1); // Without '\n'
	l->label = -1;
	memcpy(l->original, s, strlen(s) - 1);
	memcpy(l->original + (strlen(s) - 1), (char*)&se, 1);
	
	char* tend;						// Current token end pointer
	TOKENTYPE tt;						// Current token type
	int tn = 0;						// Number of tokens
	int tns = 0;						// Number of tokens scanned
	
	int i;
	i = 0;
	while (s[i++] != '\n')
		s[i - 1] = toupper(s[i - 1]);
	
	if (!(s && *s)) return NULL;
	
	while (*s && *s != '\n')
	{
		while (*s == ' ')
			s++;
		while (*s == '\t')
			s++;
		TOKEN tk;
		char* t;
		if (isDigit(*s))
		{
			int i;
			for (i = 0; i < MAX_DIGITS; i++)
			{
				if (!isDigit(s[i]))
					if (i)
					{
						tt = TNUMBER;
						tend = s + i;
						goto tokenf;
					} else {
						tt = TUNKNOWN;
						goto tokenf;
					}
			}
		}
		else if (checkOperator(s) != -1)
		{
			tt = TOPERATOR;
			tend = s + 1;
		}
		else
		{
			tend = s;
			while ((*tend >= 'A') && (*tend <= 'Z'))
				tend++;
			char* t = (char*)malloc((tend - s) + 1);
			memcpy(t, s, tend - s);
			memcpy(t + (tend - s), (char*)&se, 1);
			KEYWORD kr = checkKeyword(t);
			if (kr != UNKNOWN) tt = TKEYWORD; else tt = TVARNAME;
		}

	    tokenf:
		tk.t = tt;
		switch (tt)
		{
			case TNUMBER:
				t = (char*)malloc((tend - s) + 1);
				memcpy(t, s, tend - s);
				memcpy(t + (tend - s), (char*)&se, 1);
				if (!tns)
				{
					l->label = atoi(t);
					if (atoi(t) > maxLabel) maxLabel = atoi(t);
					goto endc;
				}
				tk.number = atoi(t);
				break;
			case TKEYWORD:
				t = (char*)malloc((tend - s) + 1);
				memcpy(t, s, tend - s);
				memcpy(t + (tend - s), (char*)&se, 1);
				tk.keyword = checkKeyword(t);
				break;
			case TOPERATOR:
				tk.operator = checkOperator(s);
				break;
			default: // Varname or Unknown
				t = (char*)malloc((tend - s) + 1);
				memcpy(t, s, tend - s);
				memcpy(t + (tend - s), (char*)&se, 1);
				tk.varname = (char*) malloc(sizeof(t));
				strcpy(tk.varname, t);
				break;
		}
		l->tokens = (TOKEN*) realloc(l->tokens, (tn + 1) * sizeof(TOKEN));
		l->tokens[tn] = tk;
		tn++; tns++;
	    endc:
		s = tend;
		tns++;
	}
	
	l->tokensnum = tn;
	
	return l;
}

int runParser(FILE* f)
{
	int ln;
	int fln;
	char line[256];
	
	if (!f) return -1;
	
	ln = 0;
	fln = 1;
	lines = NULL;
	
	while (fgets(line, sizeof(line), f) != NULL)
	{
		LINE* get = parseLine((char*)&line);
		if (get->tokensnum)
		{
			get->originaln = fln;
			lines = (LINE*)realloc(lines, (ln + 1) * sizeof(LINE));
			memcpy(&lines[ln++], get, sizeof(LINE));
		}
		fln++;
	}
	
	return ln;
}

int lineToASM(FILE* f, LINE* al, int linen)
{
	LINE* l = &al[linen];
	TOKEN* toks = l->tokens;
	if (commentCode) fprintf(f, "; %s\n", l->original);
	if (existLabel(linen)) fprintf(f, "label %i\n", getLabelNum(linen));
	if (al[linen].label != -1) fprintf(f, "label %i\n", al[linen].label);
	if (l->tokensnum < 1) return 1;
	if (toks[0].t == TKEYWORD)
	{
		KEYWORD k = toks[0].keyword;
		if (k == PRINT)
		{
			if (l->tokensnum < 2) return 6;
			if (toks[1].t == TNUMBER)
				fprintf(f, "debug %i\n", toks[1].number);
			else
				fprintf(f, "debug _t%i\n", getVarNum(toks[1].varname));
		}
		else if (k == IF)
		{
			if (l->tokensnum != 5) return 12;
			if (toks[1].t != TNUMBER && toks[1].t != TVARNAME) return 13;
			if (toks[2].t != TOPERATOR) return 14;
			if (toks[3].t != TNUMBER && toks[3].t != TVARNAME) return 15;
			if (toks[4].t != TKEYWORD) return 16;
			if (toks[4].keyword != THEN) return 17;
			if (toks[2].operator == EQUAL)
				fprintf(f, "cmp ");
			else if (toks[2].operator == LOW)
				fprintf(f, "low ");
			else if (toks[2].operator == HIGH)
				fprintf(f, "high ");
			else
				return 18;
			if (toks[1].t == TNUMBER)
				fprintf(f, "%i ", toks[1].number);
			else if (toks[1].t == TVARNAME)
				fprintf(f, "_t%i ", getVarNum(toks[1].varname));
			else
				return 19;
			if (toks[3].t == TNUMBER)
				fprintf(f, "%i ", toks[3].number);
			else if (toks[3].t == TVARNAME)
				fprintf(f, "_t%i ", getVarNum(toks[3].varname));
			else
				return 20;
			fprintf(f, "_t%i\n", getTempVar());
			NUMBER l = 0;
			NUMBER o = 1;
			int i; for (i = linen + 1; i < linesn; i++)
			{
				if (al[i].tokens[0].t != TKEYWORD) continue;
				if (al[i].tokens[0].keyword == IF) o++;
				if (al[i].tokens[0].keyword == END) o--;
				if (al[i].tokens[0].keyword == ELSE || al[i].tokens[0].keyword == END) al[i].oi = o;
				if (al[i].tokens[0].keyword == ELSE) if (o == 1) { l = i; break; }
				if (al[i].tokens[0].keyword == END) if (!o) { l = i; break; }
			}
			if (!l) return 21;
			l++;
			if (!existLabel(l)) addLabel(l);
			fprintf(f, "cmpjump _t%i 0 %i 0\n", getTempVar(), getLabelNum(l));
		}
		else if (k == ELSE)
		{
			NUMBER l = 0;
			NUMBER o = al[linen].oi;
			int i; for (i = linen + 1; i < linesn; i++)
			{
				if (al[i].tokens[0].t != TKEYWORD) continue;
				if (al[i].tokens[0].keyword == IF) o++;
				if (al[i].tokens[0].keyword == END) o--;
				if (al[i].tokens[0].keyword == END)
				{
					if (!o) { l = i; break; }
				}
			}
			if (!l) return 22;
			if (!existLabel(l)) addLabel(l);
			fprintf(f, "jump %i 0\n", getLabelNum(l));
		}
		else if (k == FOR)
		{
			if (l->tokensnum != 6 && l->tokensnum != 8) return 22;
			if (toks[1].t != TVARNAME) return 23;
			if (toks[2].t != TOPERATOR) return 24;
			if (toks[2].operator != EQUAL) return 25;
			if (toks[3].t != TNUMBER && toks[3].t != TVARNAME) return 26;
			if (toks[4].t != TKEYWORD) return 27;
			if (toks[4].keyword != TO) return 28;
			if (toks[5].t != TNUMBER && toks[5].t != TVARNAME) return 29;
			if (l->tokensnum == 8)
			{
				if (toks[6].t != TKEYWORD) return 30;
				if (toks[6].keyword != STEP) return 31;
				if (toks[7].t != TNUMBER && toks[7].t != TVARNAME) return 32;
			}
			if (!existVariable(toks[1].varname)) addVariable(toks[1].varname);
		}
		else if (k == NEXT)
		{
			
		}
		else if (k == GOTO)
		{
			if (l->tokensnum != 2) return 33;
			if (toks[1].t != TNUMBER && toks[1].t != TVARNAME) return 34;
			if (toks[1].t == TNUMBER)
				fprintf(f, "jump %i 0\n", toks[1].number);
			else if (toks[1].t == TVARNAME)
				if (existUserLabel((char*)toks[1].varname)) fprintf(f, "jump %i 0\n", getUserLabelNum((char*)toks[1].varname));
				else fprintf(f, "jump _t%i 0\n", getVarNum(toks[1].varname));
			else
				return 35;
		}
		else if (k == END)
		{
			if (!existLabel(linen)) fprintf(f, "exit\n");
		}
		else
		{
			return 9;
		}
	}
	else if (toks[0].t == TVARNAME)
	{
		if (l->tokensnum < 2) return 7;
		if (toks[1].t != TOPERATOR) return 3;
		if (toks[1].operator == EQUAL)
		{
			if (l->tokensnum < 3) return 8;
			if (!existVariable(toks[0].varname)) addVariable(toks[0].varname);
			if (l->tokensnum > 3)
			{
				if (toks[2].t == TNUMBER)
					fprintf(f, "assign _t%i %i\n", getVarNum(toks[0].varname), toks[2].number);
				else if (toks[2].t == TVARNAME)
					fprintf(f, "move _t%i _t%i\n", getVarNum(toks[0].varname), getVarNum(toks[2].varname));
				else
					return 8;
				int i; for (i = 3; i < l->tokensnum; i+=2)
				{
					if (toks[i].t != TOPERATOR) return 9;
					if (l->tokensnum < (i + 1)) return 10;
					if (toks[i].operator == ADD)
						fprintf(f, "add _t%i ", getVarNum(toks[0].varname));
					else if (toks[i].operator == SUB)
						fprintf(f, "sub _t%i ", getVarNum(toks[0].varname));
					else if (toks[i].operator == MUL)
						fprintf(f, "mul _t%i ", getVarNum(toks[0].varname));
					else if (toks[i].operator == DIV)
						fprintf(f, "div _t%i ", getVarNum(toks[0].varname));
					else
						return 11;
					if (toks[i + 1].t == TNUMBER)
						fprintf(f, "%i", toks[i + 1].number);
					else if (toks[i + 1].t == TVARNAME)
						fprintf(f, "_t%i", getVarNum(toks[i + 1].varname));
					else
						return 8;
					fprintf(f, " _t%i\n", getVarNum(toks[0].varname));
				}
			} else {
				if (toks[2].t == TNUMBER)
					fprintf(f, "assign _t%i %i\n", getVarNum(toks[0].varname), toks[2].number);
				else if (toks[2].t == TVARNAME)
					fprintf(f, "move _t%i _t%i\n", getVarNum(toks[0].varname), getVarNum(toks[2].varname));
				else
					return 8;
			}
		}
		else if (toks[1].operator == DP)
		{
			if (existUserLabel((char*)toks[0].varname)) return 41;
			addUserLabel((char*)toks[0].varname);
			fprintf(f, "label %i\n", getUserLabelNum((char*)toks[0].varname));
		}
		else
		{
			return 40;
		}
	}
	else
	{
		return 5;
	}
	
	if (commentCode) fprintf(f, "\n");
	
	return 0;
}

int compileAll(FILE* in, FILE* o)
{
	printf("Compile started.\n");
	if (!in || !o) return 1;
	printf("Parsing...\n");
	linesn = runParser(in);
	printf("Done, %i lines parsed\n", linesn);
	if (!lines) { printf("Error: NULL pointer!\n"); return 1; }
	printf("Generating SASM file...\n");
	int i;
	for (i = 0; i < linesn; i++)
	{
		int r = lineToASM(o, lines, i);
		if (r) printf("Error parsing line [%i]: %i\n", lines[i].originaln, r);
	}
	return 0;
}


