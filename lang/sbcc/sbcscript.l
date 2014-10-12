%{
#include <iostream>
#include <string>
using namespace std;
#define YY_DECL extern "C" int yylex()
#include "ast.h"
#include "sbcscript.tab.h"

int line_num = 1;
string string_buf;
#define _T() yylval.sval = strdup(yytext)

%}
%option noyywrap
%x COMMENT
%x STR
%%
\/\/           { BEGIN(COMMENT); }
<COMMENT>\n    { ++line_num; BEGIN(INITIAL); }
<COMMENT>.     ;
[ \t]          ;
\"             { string_buf="\""; BEGIN(STR); }
<STR>\\\"      { string_buf += "\\\""; }
<STR>\n        { BEGIN(INITIAL); }  /* TODO add error */
<STR>\"        { string_buf += "\""; 
                 yylval.sval = strdup(string_buf.c_str()); 
                 BEGIN(INITIAL); 
                 return STRVAR; } 
<STR>.         { string_buf += yytext[0]; }
int            { yylval.ival=1; return VAR; }
void           { yylval.ival=0; return VOID; }
thread         { return THREAD; }
stop           { return STOP; }
is_running     { return IS_RUNNING; }
do             { return DO; }
while          { return WHILE; }
for            { return FOR; }
exit           { return EXIT; }
if             { return IF; }
else           { return ELSE; }
return         { return RETURN; }
debug          { return DEBUG; }
error          { return ERROR; }
print          { return PRINT; }
\"[
wait           { return WAIT; }
user[0-9]+     { yylval.ival = atoi(yytext+4); return USERF; }
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

