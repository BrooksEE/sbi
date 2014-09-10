%{
#include <iostream>
//#include <string>
using namespace std;
#define YY_DECL extern "C" int yylex()
#include "ast.h"
#include "sbcscript.tab.h"

int line_num = 1;
#define _T() yylval.sval = strdup(yytext)

%}
%option noyywrap
%x COMMENT
%%
\/\/           { BEGIN(COMMENT); }
<COMMENT>\n    { ++line_num; BEGIN(INITIAL); }
<COMMENT>.     ;
[ \t]          ;
int            { return VAR; }
thread         { return THREAD; }
function       { return FUNCTION; }
while          { return WHILE; }
if             { return IF; }
else           { return ELSE; }
return         { return RETURN; }
debug          { return DEBUG; }
error          { return ERROR; }
wait           { return WAIT; }
==             { return EQOP; }
\!=            { return NEOP; }
\<=            { return LE; }
\>=            { return GE; }
[0-9]+         { yylval.ival = atoi(yytext); return INT; }
0x[0-9a-f]+    { yylval.ival = strtol( yytext, NULL, 16 ); return INT; }
(_[a-zA-Z0-9]+)|([a-zA-Z][a-zA-Z0-9_]*)    {
                    _T();
                    return STRING;
}
[;\(\)\!={}\+\<\>\-/\*,]     { 
                /* cout << "token: '" << yytext[0] << "'" << endl; */ return yytext[0]; }
\n            { ++line_num;}
.             { cout << "warn ignore " << yytext << endl; } 
%%
