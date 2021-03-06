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
    VarDec *vardec;
    FunctionArgList *funcargs;
    FunctionCallArgList *funccallargs;
    PrintArgList *printargs;
    Stmts *stmts;
}

%token THREAD DEBUG ERROR PRINT WAIT EXIT
%token WHILE IF ELSE RETURN STOP IS_RUNNING DO FOR
%token EQOP NEOP GE LE LOGIC_AND LOGIC_OR

%token <ival> INT VAR VOID USERF
%token <sval> STRING STRVAR


%type<node> sbcscript stmt code_block function_decl assign expr term return deberr print
%type<node> gvar_decl var_decl function_call userfunc_call while if ifblock elseblock wait thread stop is_running 
%type<node> dowhile for for_expr for_begin for_step print_arg
%type<funcargs> function_decl_args 
%type<funccallargs> function_call_args
%type<printargs> print_args
%type<stmts> stmts
%type<sval> lval

%left '<' '>' LE GE EQOP NEOP LOGIC_AND LOGIC_OR
%left '-' '+'
%left '*' '/' '%'

%%
sbcscript:
    sbcscript function_decl { gProgram->m_functions->push_back((Function*)$2); }
    | sbcscript gvar_decl { gProgram->m_globals->push_back((VarDec*)$2); }
    | gvar_decl { gProgram = new Program(); gProgram->m_globals->push_back((VarDec*)$1); }
    | function_decl { gProgram = new Program(); gProgram->m_functions->push_back((Function*)$1); }
    ;

gvar_decl:
    VAR STRING ';' { $$ = new VarDec($2); free($2); } 
    | VAR STRING '=' INT ';' { IntExpr *v = new IntExpr($4); $$ = new VarDec($2, v); free($2); }
    ;

function_decl:
    VAR STRING '(' function_decl_args ')' code_block { $$=new Function($1, $2, $4, (Block*)$6); free($2); }
    | VOID STRING '(' function_decl_args ')' code_block { $$=new Function($1, $2, $4, (Block*)$6); free($2); }
    ;

function_decl_args:
    /* none */ { $$ = new FunctionArgList(); }
    | VAR STRING { $$ = new FunctionArgList(); $$->push_back($2); free($2); }
    | function_decl_args ',' VAR STRING {$$->push_back($4); free($4); }
    ;

code_block:
    '{' stmts '}' { $$=new Block( $2 ); }

stmts: 
    /* none */ { $$= new Stmts(); }
    | stmt { $$=new Stmts(); $$->push_back($1); }
    | stmts stmt { $1->push_back($2); }
    ;

stmt:
    assign ';' { $$ = $1; }
    | var_decl ';' { $$ = $1; }
    | while { $$ = $1; }
    | dowhile { $$ = $1; }
    | for { $$ = $1; }
    | function_call ';' { $$ = $1; }
    | if { $$ = $1; }
    | return ';' { $$ = $1; }
    | deberr ';' { $$ = $1; }
    | print ';' { $$ = $1; }
    | wait ';' { $$ = $1; }
    | thread ';' { $$ = $1; }
    | userfunc_call ';' { $$ = $1; }
    | stop ';' { $$=$1; }
    | is_running ';' { $$=$1; }
    | EXIT '(' ')' ';' { $$= new ExitStmt(); }
    ;

var_decl:
    VAR STRING { $$ = new VarDec($2); free($2); }
    | VAR STRING '=' expr { $$ = new VarDec( $2, (Expr*)$4); free($2); }
    ;

assign:
    lval '=' expr { $$ = new AssignStmt ( $1, (Expr*)$3 ); free($1); }
    ;

for:
    FOR '(' for_begin';' for_expr ';' for_step ')' code_block { $$ = new ForStmt ( $3, (Expr*)$5, (Expr*)$7, (Block*)$9 ); }
    | FOR '(' for_begin ';' for_expr ';' for_step ')' stmt { $$ = new ForStmt ( $3, (Expr*)$5, (Expr*)$7, $9 ); }
    | FOR '(' for_begin ';' for_expr ';' for_step ')' ';' { $$ = new ForStmt ( $3, (Expr*)$5, (Expr*)$7 ); }
    ;

for_begin:
    /* none */ {$$=NULL;}
    | var_decl {$$=$1;}
    | assign { $$=$1; }
    ;

for_expr:
    /* none */ {$$=NULL; }
    | expr { $$ = $1; }
    ;
for_step:
    /* none */ {$$=NULL; }
    | assign { $$=$1; }
    ;

while:
    WHILE '(' expr ')' code_block { $$ = new WhileStmt( (Expr*)$3, (Block*)$5 ); }
    | WHILE '(' expr ')' stmt { $$ = new WhileStmt ( (Expr*)$3, $5 ); }
    | WHILE '(' expr ')' ';' { $$ = new WhileStmt ( (Expr*)$3 ); }
    ;

dowhile:
    DO code_block WHILE '(' expr ')' ';' { $$ = new DowhileStmt ( (Block*)$2, (Expr*)$5 ); }

if:
    ifblock { $$=$1; }
    | ifblock elseblock { $$=$1; ((IfStmt*)$$)->m_false = (Block*)$2; } 
    ;
ifblock:
    IF '(' expr ')' code_block { $$ = new IfStmt ( (Expr*)$3, (Block*)$5 ); }
    | IF '(' expr ')' stmt { Block* tmp = new Block(); tmp->m_stmts->push_back($5); $$ = new IfStmt ( (Expr*)$3, tmp ); }
    ;
elseblock:
    ELSE code_block { $$ = $2; } 
    | ELSE stmt { $$ = new Block(); ((Block*)$$)->m_stmts->push_back($2); }


return:
    RETURN { $$ = new ReturnStmt(NULL); }
    | RETURN expr { $$ = new ReturnStmt((Expr*)$2); }

thread:
    THREAD '(' STRING ')' { $$ = new ThreadExpr($3); free($3); }
    ;

wait:
    WAIT '(' STRING ')' { $$ = new WaitStmt($3); free($3); }
    ;

stop:
    STOP '(' STRING ')' { $$ = new StopStmt($3); free($3); } 
    ;

is_running:
    IS_RUNNING '(' STRING ')' { $$ = new RunningExpr($3); free ($3); }
    ;

function_call:
    STRING '(' function_call_args ')' { $$ = new FuncExpr( $1, $3 ); free($1); }
    ;

userfunc_call:
    USERF '(' function_call_args ')' { $$ = new UserfuncStmt( $1, $3 ); }

function_call_args:
    /* empty */ { $$ = new FunctionCallArgList(); }
    | expr { $$ = new FunctionCallArgList(); $$->push_back((Expr*)$1); }
    | function_call_args ',' expr { $$->push_back((Expr*)$3); }
    ;

deberr:
    DEBUG '(' expr ')' { $$ = new DebugStmt ( "debug", (Expr*)$3 ); }
    | ERROR '(' expr ')' { $$ = new DebugStmt ( "error", (Expr*)$3 ); } 
    ;

print:
    PRINT '(' print_args ')' { $$ = new PrintStmt ( $3 ); }
    ;

print_args:
    /* empty */ { $$ = new PrintArgList(); } 
    | print_arg { $$ = new PrintArgList(); $$->push_back((PrintArg*)$1); }
    | print_args ',' print_arg { $$->push_back((PrintArg*)$3); }
    ;

print_arg:
    STRVAR { $$ = new PrintArg((const char*)$1); free($1); }
    | expr { $$ = new PrintArg((Expr*)$1); }
    ;

expr:
    term { $$ = $1; }
    | '(' expr ')' { $$ = $2; }
    | expr '+' expr { $$ = new BinaryExpr ( (Expr*)$1, OP_ADD, (Expr*)$3 ); }
    | expr '-' expr { $$ = new BinaryExpr ( (Expr*)$1, OP_SUB, (Expr*)$3 ); }
    | expr '*' expr { $$ = new BinaryExpr ( (Expr*)$1, OP_MULT, (Expr*)$3 ); }
    | expr '/' expr { $$ = new BinaryExpr ( (Expr*)$1, OP_DIV, (Expr*)$3 ); }
    | expr '%' expr { $$ = new BinaryExpr ( (Expr*)$1, OP_MOD, (Expr*)$3 ); }
    | expr '<' expr { $$ = new BinaryExpr ( (Expr*)$1, OP_LT, (Expr*)$3 ); }
    | expr '>' expr { $$ = new BinaryExpr ( (Expr*)$1, OP_GT, (Expr*)$3 ); }
    | expr LE expr { $$ = new BinaryExpr ( (Expr*)$1, OP_LE, (Expr*)$3 ); }
    | expr GE expr { $$ = new BinaryExpr ( (Expr*)$1, OP_GE, (Expr*)$3 ); }
    | expr EQOP expr { $$ = new BinaryExpr ( (Expr*)$1, OP_EQ, (Expr*)$3 ); }
    | expr NEOP expr { $$ = new BinaryExpr ( (Expr*)$1, OP_NE, (Expr*)$3 ); }
    | expr LOGIC_AND expr { $$ = new BinaryExpr ( (Expr*)$1, OP_AND, (Expr*)$3 ); }
    | expr LOGIC_OR expr { $$ = new BinaryExpr ( (Expr*)$1, OP_OR, (Expr*)$3 ); }
    | '!' expr { $$ = new InvExpr( (Expr*)$2); }
    ;

term:
    INT { $$ = new IntExpr($1); }
    | STRING { $$ = new VarExpr($1); free($1); }
    | function_call { $$ = $1; }
    | thread { $$ = $1; }
    | userfunc_call { $$ = $1; } 
    | is_running { $$ = $1; }
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

