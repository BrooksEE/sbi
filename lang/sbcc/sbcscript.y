%{
#include <cstdio>
#include <iostream>
#include "ast.h"

using namespace std;

// stuff from flex that bison needs to know about:
extern "C" int yylex();
extern "C" int yyparse();
extern "C" FILE *yyin;

extern int line_num;
Program* gProgram;

#define YYDEBUG 1
 
void yyerror(const char *s);
%}

%union {
    int ival;
    char *sval;
    Node *node;
    Globals *globals;
    FunctionList *functions;
    VarDec *vardec;
    FunctionArgList *funcargs;
    FunctionCallArgList *funccallargs;
    Stmts *stmts;
    ThreadList* threads;
}

%token FUNCTION THREAD VAR DEBUG ERROR WAIT
%token WHILE IF ELSE RETURN
%token EQOP NEOP GE LE

%token <ival> INT
%token <sval> STRING


%type<node> insscript stmt code_block function_decl thread assign expr term return deberr
%type<node> gvar_decl var_decl function_call while if ifblock elseblock wait
%type<globals> globals
%type<functions> function_list
%type<funcargs> function_decl_args 
%type<funccallargs> function_call_args
%type<threads> thread_list
%type<stmts> stmts
%type<sval> lval

%left '<' '>' LE GE EQOP NEOP
%left '-' '+'
%left '*' '/'

%%
insscript:
    globals function_list thread_list { gProgram = new Program ( $1, $2, $3 ); delete $1; delete $2; delete $3; }
    ;
globals:
    /* empty */ { $$=new Globals(); }
    |globals gvar_decl { $1->push_back((VarDec*)$2); }
    | gvar_decl { $$=new Globals(); $$->push_back( (VarDec*)$1 ); }
    ;

gvar_decl:
    VAR STRING ';' { $$ = new VarDec($2); free($2); } 
    | VAR STRING '=' INT ';' { IntExpr *v = new IntExpr($4); $$ = new VarDec($2, v); }
    ;

function_list: 
    function_list function_decl { $1->push_back((Function*)$2); }
    | function_decl { $$ = new FunctionList(); $$->push_back((Function*)$1); }
    | /* empty */ { $$ = new FunctionList(); }
    ;

function_decl:
    FUNCTION STRING '(' function_decl_args ')' code_block { $$=new Function($2, $4, (Block*)$6); delete $4; delete $6; free($2); }

function_decl_args:
    /* none */ { $$ = new FunctionArgList(); }
    | STRING { $$ = new FunctionArgList(); $$->push_back($1); free($1); }
    | function_decl_args ',' STRING {$$->push_back($3); }
    ;

code_block:
    '{' stmts '}' { $$=new Block( $2 ); delete $2; }

thread_list:
    thread { $$ = new ThreadList(); $$->push_back((Thread*)$1); }
    | thread_list thread { $1->push_back((Thread*)$2); }
    ;

thread:
    THREAD STRING code_block { $$ = new Thread($2, (Block*)$3); delete $3; free($2); }
    ;

stmts: 
    /* none */ { $$= new Stmts(); }
    | stmt { $$=new Stmts(); $$->push_back($1); }
    | stmts stmt { $1->push_back($2); }
    ;

stmt:
    assign { $$ = $1; }
    | var_decl { $$ = $1; }
    | while { $$ = $1; }
    | function_call ';' { $$ = new FuncCallStmt((FuncExpr*)$1); }
    | if { $$ = $1; }
    | return { $$ = $1; }
    | deberr { $$ = $1; }
    | wait { $$ = $1; }
    ;

var_decl:
    VAR STRING ';' { $$ = new VarDec($2); free($2); }
    | VAR STRING '=' expr ';' { $$ = new VarDec( $2, (Expr*)$4); free($2); }
    ;

assign:
    lval '=' expr ';' { $$ = new AssignStmt ( $1, (Expr*)$3 ); free($1); }
    ;

while:
    WHILE '(' expr ')' code_block { $$ = new WhileStmt( (Expr*)$3, (Block*)$5 ); delete $5;}
    | WHILE '(' expr ')' stmt { $$ = new WhileStmt ( (Expr*)$3, $5 ); }
    | WHILE '(' expr ')' ';' { $$ = new WhileStmt ( (Expr*)$3 ); }
    ;

if:
    ifblock { $$=$1; } /* TODO memory management is really really bad in this file clear et al */
    | ifblock elseblock { $$=$1; ((IfStmt*)$$)->m_false = *(Block*)$2; ((Block*)$2)->m_stmts.clear(); delete $2; } 
    ;
ifblock:
    IF '(' expr ')' code_block { $$ = new IfStmt ( (Expr*)$3, (Block*)$5 ); delete $5; }
    | IF '(' expr ')' stmt { Block tmp; tmp.m_stmts.push_back($5); $$ = new IfStmt ( (Expr*)$3, &tmp ); }
    ;
elseblock:
    ELSE code_block { $$ = $2; } 
    | ELSE stmt { $$ = new Block(); ((Block*)$$)->m_stmts.push_back($2); }


return:
    RETURN ';' { $$ = new ReturnStmt(NULL); }
    | RETURN expr ';' { $$ = new ReturnStmt((Expr*)$2); }

wait:
    WAIT '(' STRING ')' ';' { $$ = new WaitStmt($3); }
    ;

function_call:
    STRING '(' function_call_args ')' { $$ = new FuncExpr( $1, $3 ); delete $3; free($1); }
    ;

function_call_args:
    expr { $$ = new FunctionCallArgList(); $$->push_back((Expr*)$1); }
    | function_call_args ',' expr { $$->push_back((Expr*)$3); }
    ;

deberr:
    DEBUG '(' expr ')' ';' { $$ = new DebugStmt ( "debug", (Expr*)$3 ); }
    | ERROR '(' expr ')' ';' { $$ = new DebugStmt ( "error", (Expr*)$3 ); } 
    ;

expr:
    term { $$ = $1; }
    | '(' expr ')' { $$ = $2; }
    | expr '+' expr { $$ = new BinaryExpr ( (Expr*)$1, OP_ADD, (Expr*)$3 ); }
    | expr '-' expr { $$ = new BinaryExpr ( (Expr*)$1, OP_SUB, (Expr*)$3 ); }
    | expr '*' expr { $$ = new BinaryExpr ( (Expr*)$1, OP_MULT, (Expr*)$3 ); }
    | expr '/' expr { $$ = new BinaryExpr ( (Expr*)$1, OP_DIV, (Expr*)$3 ); }
    | expr '<' expr { $$ = new BinaryExpr ( (Expr*)$1, OP_LT, (Expr*)$3 ); }
    | expr '>' expr { $$ = new BinaryExpr ( (Expr*)$1, OP_GT, (Expr*)$3 ); }
    | expr LE expr { $$ = new BinaryExpr ( (Expr*)$1, OP_LE, (Expr*)$3 ); }
    | expr GE expr { $$ = new BinaryExpr ( (Expr*)$1, OP_GE, (Expr*)$3 ); }
    | expr EQOP expr { $$ = new BinaryExpr ( (Expr*)$1, OP_EQ, (Expr*)$3 ); }
    | expr NEOP expr { $$ = new BinaryExpr ( (Expr*)$1, OP_NE, (Expr*)$3 ); }
    | '!' expr { $$ = new InvExpr((Expr*)$2); }
    ;

term:
    INT { $$ = new IntExpr($1); }
    | STRING { $$ = new VarExpr($1); free($1); }
    | function_call { $$ = $1; }
    ;

lval:
    STRING
    ;

%%
void usage(char* prog) {
   cout << "Usage: " << prog << " <input file>" << endl;
}
int main(int argc, char* argv[]) {
    if (argc<2) {
       usage(argv[0]);
       return 1;
    }
    FILE *input = fopen(argv[1], "r");
    if (!input) {
        cout << "File not found: " << argv[1] << endl;
        usage(argv[0]);
        return 1;
    }
    //yydebug=argc>2?1:0;
    yyin = input;

    do {
        yyparse();
    } while (!feof(yyin));
    
    CodeCtx ctx(argc>2?true:false);
    gProgram->genCode(ctx);
    delete gProgram;
}

void yyerror(const char *s) {
    cout << "Parse error line: " << line_num << " Message: " << s << endl;
    // might as well halt now:
    exit(-1);
}
