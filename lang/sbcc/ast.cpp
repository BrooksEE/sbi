
#include "ast.h"

#include <map>

#include <stdio.h>
#include <stdarg.h>
#include <typeinfo>
#include <cstring>

using namespace std;


#define GLOBALS_MAX 64 // 64 from sbi defs

typedef map<string,string> VarCtx;
typedef vector<VarCtx*> VarScope;

typedef map<string,int> LabelNums;

class CtxImpl {
 public:
    bool debug;
    int err_cnt;

    VarScope varScope;
    uint8_t regStack; // can go from 0-15

    LabelNums labels; 
    int nextLabel;
    int nextGlobal;

    map<string,int> func_types; // function return types (0=void 1=int)
    Function *cur_func; // current functio being generated if needed by other embedded statements.


    CtxImpl() :
        debug(false),
        err_cnt(0),
        regStack(2), // start at 2 to reserve _r0 and _r1 for expressions.
        nextLabel(0),
        nextGlobal(0) {
        PushVarCtx(); // global scope
    }
    ~CtxImpl() {
        while (varScope.size()) PopVarCtx();
    }

    // variable management
    void PushVarCtx() {
       varScope.push_back(new VarCtx());
    }
    void PopVarCtx() {
       VarCtx* back=varScope.back();
       varScope.pop_back();
       if (back->size()) // if 0 then we don't use regs
          regStack -= back->size();
       delete back;
    }
    VarCtx* CurVarCtx() { return varScope.back(); } 
    
    void AddVar(const string &var) {
       VarCtx* ctx = CurVarCtx();
       // if has already error
       if (ctx->count(var)>0) {
            error ( "variable (%s) already defined.", var.c_str() );
       }
       
       // add var to context
       char rbuf[5];
       if (varScope.size()==1) {
           // in this case, go ahead and just use a free global variable
           // instead of registers.
           if (nextGlobal > GLOBALS_MAX-1) {
              error ( "global variable overflow." );
           }
           snprintf(rbuf,5,"_t%d", nextGlobal++);
       } else {
          // put variable in current scope. 
          if (regStack>15) { // TODO maybe get rid of register concept and just use the stack.
               error ( "Too many local variables." );
          }
          snprintf(rbuf,5,"_r%d",regStack++);
       }
       (*ctx)[var] = string(rbuf);
       
    }

    string FindVarLoc(const string &var) {
       VarScope::reverse_iterator itr;
       for (itr=varScope.rbegin();
            itr<varScope.rend();
            ++itr) {
            VarCtx* ctx = *itr;
            if (ctx->count(var)>0) {
                return (*ctx)[var];
            }
       }

       error ( "Undefined variable: %s", var.c_str() );
       return "error";
    }


    // labels
    void AddFunction(const Function *func) {
        if ( labels.count(func->m_name)>0) {
            error ( "Function name (%s) already exists.", func->m_name.c_str() );
        }
        labels[func->m_name] = nextLabel++; 
        func_types[func->m_name] = func->m_rettype;
    }

    int GetFunctionLoc(const string &name) {
        if ( labels.count(name)==0 ) error ( "Function (%s) not defined.", name.c_str() );
        return labels[name];
    }

    int GetFunctionRet(const string &name) {
        if (func_types.count(name) == 0) error ( "Function (%s) not defined.", name.c_str() );
        return func_types[name];
    }

    int NextLabel() {
        return nextLabel++;
    }

    // error management
    void error(const char* err, ...) {
        int l = strlen(err) + 256;
        char *s = new char[l];
        va_list args;
        va_start(args,err);
        vsnprintf ( s, l, err, args );
        va_end(args);
        cerr << "ERROR: " << s << endl;
        delete [] s;
        ++err_cnt;
    }
    void warning (const char* warn, ...) {
        int l = strlen(warn) + 256;
        char *s = new char[l];
        va_list args;
        va_start(args,warn);
        vsnprintf ( s, l, warn, args );
        va_end(args);
        cerr << "WARNING: " << s << endl;
        delete [] s;
    }
};

void Node::emit(CodeCtx &ctx, const char* format, ...) {
    static char out[256];
    va_list args;
    va_start(args, format);
    vsnprintf( out, 256, format, args );
    va_end(args);

    cout << out; // << endl;
}

#define CTX ((CtxImpl*)ctx.impl)
#define DEBUG(x) if (CTX->debug) { cout << "\t\t\t; "; x; cout << endl; }


void Node::genCode(CodeCtx &ctx) {
    CTX->error ( " ** Node code generation not implemented ** " );
}

std::ostream& operator << (std::ostream &o, const Expr &e) {
   e.stream(o);
   return o;
}

CodeCtx::CodeCtx(bool debug) {
  CtxImpl *ctx = new CtxImpl();
  ctx->debug = debug;
  impl=ctx;
}
CodeCtx::~CodeCtx() {
  CtxImpl *p = (CtxImpl*)impl; 
  delete p;
}

Program::~Program() {
  for (Globals::iterator itr = m_globals->begin();
       itr < m_globals->end();
       ++itr) {
       delete *itr;
  }
  for (FunctionList::iterator itr = m_functions->begin();
       itr < m_functions->end();
       ++itr) {
       delete *itr;
  }

  delete m_globals;
  delete m_functions;
  
}

void Program::genCode(CodeCtx &ctx) {
  DEBUG(cout << "Generate a program." << endl);
  
  for (Globals::iterator itr = m_globals->begin();
       itr < m_globals->end();
       ++itr) {
       (*itr)->genCode(ctx);
  }
  // pre compute label names
  for (FunctionList::iterator itr = m_functions->begin();
       itr < m_functions->end();
       ++itr) {
       CTX->AddFunction(*itr);
  }
  
  // generate a main

  int m = CTX->GetFunctionLoc("main");
  emit ( ctx, "jump %d 1\t\t\t; jump to main\n", m );
  emit ( ctx, "exit\n" );
  
  for (FunctionList::iterator itr = m_functions->begin();
       itr < m_functions->end();
       ++itr ) {
       (*itr)->genCode(ctx);
  }

}


void VarDec::genCode(CodeCtx &ctx) {
  if (CTX->debug) {
    cout << "\t\t\t; " << m_name ;
    if (m_value) 
        cout << *m_value;
    cout << ":" << endl;
  }
  CTX->AddVar(m_name);
  
  string loc = CTX->FindVarLoc(m_name);
  if (m_value) {
     emit ( ctx, "; var decl %s=%s\n", m_name.c_str(), loc.c_str() ); 
     m_value->evalTo(ctx, loc); 
  } else {
     emit ( ctx, "; Reserve space %s for variable %s\n", loc.c_str(), m_name.c_str() );
  }
}

void Block::genCode(CodeCtx &ctx) {
  DEBUG ( cout << "{" );
  CTX->PushVarCtx();
  for (Stmts::iterator itr = m_stmts->begin();
       itr < m_stmts->end();
       ++itr) {
       (*itr)->genCode(ctx);
  }
  CTX->PopVarCtx();
  DEBUG( cout << '}' );
}

Block::~Block() {
  for (Stmts::iterator itr = m_stmts->begin();
       itr < m_stmts->end();
       ++itr ) delete *itr;
  delete m_stmts;
}


void Function::genCode(CodeCtx &ctx) {
  DEBUG ( cout << "function " << m_name << '(' );
  DEBUG ( 
     for ( FunctionArgList::iterator itr = m_args->begin();
           itr < m_args->end();
           ++itr ) cout << *itr << ',';
     cout << ')' );

  int funcStart = CTX->GetFunctionLoc(m_name);
  emit ( ctx, "label %d\t\t\t; function %s\n", funcStart, m_name.c_str() );
  CTX->PushVarCtx(); 

  if (m_name == "main" && m_args->size())
    CTX->error ( "Function main does not take arguments." );

  for (FunctionArgList::iterator itr = m_args->begin();
       itr < m_args->end();
       ++itr) {
       CTX->AddVar(*itr);
  }
  for (FunctionArgList::reverse_iterator itr = m_args->rbegin();
       itr < m_args->rend();
       ++itr) {
       string loc = CTX->FindVarLoc(*itr);
       emit ( ctx, "pop %s\t\t\t; arg %s\n", loc.c_str(), itr->c_str() ); 
  }
  CTX->cur_func = this;
  m_block->genCode(ctx);
  CTX->PopVarCtx();
  // is last statement a return
  bool ret=true;
  if (m_block->m_stmts->size()) {
     Node* last = m_block->m_stmts->back(); 
     if (typeid(*last) == typeid(ReturnStmt))
        ret=false;
  }
  if (ret && m_rettype) {
    CTX->warning ( "Control of non-void function (%s) reached end without return statement.", m_name.c_str() );
    emit(ctx, "push 0\t\t\t; no return in code\n" );
  }

  if (ret)
    emit(ctx, "ret\t\t\t; return\n" );
}

// statements
void AssignStmt::genCode(CodeCtx &ctx) {
 DEBUG ( cout << m_lval << '=' << *m_val << endl );
 // TODO special lval cases
 
 string loc = CTX->FindVarLoc(m_lval);
 m_val->evalTo(ctx, loc);

}

void UserfuncStmt::genCode(CodeCtx &ctx) {
   
   CTX->PushVarCtx(); // var ctx for userarg vars
   char arg_buf[25];
   vector<string> locs;
 
   // push args in reverse order
   int argn=0;
   for (FunctionCallArgList::iterator itr = m_args->begin();
        itr < m_args->end();
        ++itr, ++argn) {
        snprintf ( arg_buf, 225, "%darg", argn );
        CTX->AddVar ( arg_buf );
        string loc = CTX->FindVarLoc(arg_buf);
        locs.push_back(loc);
        (*itr)->evalTo(ctx, loc);
   }
   emit ( ctx, "sint %d\t\t\t; user func %d\n", m_ncall, m_ncall );
   if (m_evalTo.size()) {
     emit ( ctx, "intr %s", m_evalTo.c_str() );
   } else {
     emit ( ctx, "int" );
   }
   
   for (int i=0;i<m_args->size();++i) {
       string loc = locs.at(i);
       emit (ctx, " %s", loc.c_str() );
   }

   emit (ctx, "\t\t\t; call func with args\n" );

   CTX->PopVarCtx();
}

void UserfuncStmt::evalTo(CodeCtx &ctx, const std::string &to) {
    m_evalTo = to;
    genCode(ctx);
}



void WhileStmt::genCode(CodeCtx &ctx) {
 DEBUG ( cout << "While (" << *m_expr << ") " ); 
  int whileStart = CTX->NextLabel();
  int whileEnd = CTX->NextLabel();
  emit ( ctx, "label %d\t\t\t; while\n",whileStart ); 
  m_expr->evalTo(ctx, "_r0");
  emit ( ctx, "cmpjump 0 _r0 %d 0\t\t\t; if 0 jump to end while\n", whileEnd );
  if (m_block) m_block->genCode(ctx);
  emit ( ctx, "jump %d 0\t\t\t; loop\n", whileStart );
  emit ( ctx, "label %d\t\t\t; end while\n", whileEnd );
}

void ForStmt::genCode(CodeCtx &ctx) { 
    DEBUG ( "for (...) {" );
   
   CTX->PushVarCtx();
   if (m_begin) m_begin->genCode(ctx); 

   int forStart = CTX->NextLabel();
   emit ( ctx, "label %d\t\t\t; for ...\n", forStart );

   if (m_block) m_block->genCode(ctx);

   if (m_step) m_step->genCode(ctx);

   int forEnd = CTX->NextLabel();

   if (m_cond) {
       m_cond->evalTo(ctx, "_r0");
       emit ( ctx, "cmpjump _r0 0 %d 0\t\t\t; for condition\n", forEnd );
   }
   emit ( ctx, "jump %d 0\t\t\t; loop\n", forStart );
   emit ( ctx, "label %d:\t\t\t; end for\n", forEnd );
   CTX->PopVarCtx();

   DEBUG ( "} // end for" );
        
}

void DowhileStmt::genCode(CodeCtx &ctx) {
 DEBUG ( "do {" );
 int whileStart = CTX->NextLabel();
 int whileEnd = CTX->NextLabel();
 emit ( ctx, "label %d\t\t\t; do ", whileStart );
 m_block->genCode(ctx);
 m_expr->evalTo(ctx,"_r0");
 emit ( ctx, "cmpjump 0 _r0 %d 0\t\t\t; if 0 jump to end while\n", whileEnd );
 emit ( ctx, "jump %d 0\t\t\t; loop\n", whileStart );
 emit ( ctx, "label %d\t\t\t; end do..while\n", whileEnd );
}

void IfStmt::genCode(CodeCtx &ctx) {
 DEBUG ( cout << "If (" << *m_eval << ") " );
 int ifStart = CTX->NextLabel();
 emit ( ctx, "label %d\t\t\t; if ..\n", ifStart );
 int ifEnd = CTX->NextLabel(); 
 m_eval->evalTo(ctx,"_r0");
 emit ( ctx, "cmpjump _r0 0 %d 0\t\t\t; false -> else\n", ifEnd );
 if (m_true) m_true->genCode(ctx);
 int elseEnd=0; // unused unless else
 if ( m_false && m_false->m_stmts->size() ) {
    elseEnd = CTX->NextLabel();
    // still in the if block so jump to the end
    emit ( ctx, "jump %d 0\t\t\t; skip else\n", elseEnd );
 }
 emit ( ctx, "label %d\t\t\t; end if\n", ifEnd );
 if (m_false && m_false->m_stmts->size() ) {
    DEBUG ( cout << " else " );
    m_false->genCode(ctx);
    emit ( ctx, "label %d\t\t\t; end else\n", elseEnd );
 }

}

void ReturnStmt::genCode(CodeCtx &ctx) {
 DEBUG ( cout << "return "; 
         if (m_expr) cout << *m_expr; );
 int func_ret = CTX->cur_func->m_rettype;
 if (m_expr) {
    if (!func_ret)
        CTX->warning ( "return value from void function (%s) ignored.", CTX->cur_func->m_name.c_str() );
    else {
        m_expr->evalTo(ctx,"_r0");
        emit (ctx, "push _r0\t\t\t; return value\n" );
    }
 } else {
    if (func_ret) { 
        CTX->warning ( "return stmt without value from non-void function (%s).", CTX->cur_func->m_name.c_str() );
        emit (ctx,"push 0\t\t\t; assumed return value (bad)\n" );
    }
 }
 emit(ctx,"ret\t\t\t;\n" );
}

void DebugStmt::genCode(CodeCtx &ctx) {
  DEBUG ( cout << "debug ( " << *m_expr << " ) " );
  m_expr->evalTo(ctx, "_r0");
  emit ( ctx, "%s _r0\n", m_call.c_str() );
}

void PrintStmt::genCode(CodeCtx& ctx) {
  DEBUG ( cout << "print(" << m_str << ")" );
  emit ( ctx, "print %s;\n" , m_str.c_str() );
}

void WaitStmt::genCode(CodeCtx &ctx) {
  DEBUG ( cout << "wait ( " << m_wait << " ) " );
  string loc = CTX->FindVarLoc(m_wait);
  emit (ctx, "wait %s\t\t\t; wait %s\n", loc.c_str(), m_wait.c_str() );
}

void StopStmt::genCode(CodeCtx &ctx) {
  DEBUG ( cout << "stop ( " << m_stop << " ) ");
  string loc = CTX->FindVarLoc(m_stop);
  emit( ctx, "stop %s\t\t\t; stop %s\n", loc.c_str(), m_stop.c_str() );
}

void ExitStmt::genCode(CodeCtx &ctx) {
  DEBUG ( cout << "exit()" );
  emit ( ctx, "exit\t\t\t; exit program\n" );
}


// expressions
void Expr::genCode(CodeCtx &ctx) {
  CTX->error(" ** expression should use eval (compiler error) ** " );
}
void Expr::evalTo(CodeCtx &ctx, const string &target) {
  CTX->error(" ** evalTo not implemented ** " );
}

void IntExpr::evalTo(CodeCtx &ctx, const string &target) {
   emit ( ctx, "move %s %d;\t\t\tInt Expr\n", target.c_str(), m_val );
}

void VarExpr::evalTo(CodeCtx &ctx, const string &target) {
   string loc = CTX->FindVarLoc(m_var);
   emit ( ctx, "move %s %s\t\t\t; value of %s\n", target.c_str(), loc.c_str(), m_var.c_str() );
}

void BinaryExpr::stream(std::ostream &o) const {
 o << '(' << *m_left << ' ';
 switch(m_oper) {
   case OP_ADD: 
      o << '+';
      break;
   case OP_SUB:
      o << '-';
      break;
   case OP_MULT:
      o << '*';
      break;
   case OP_DIV:
      o << '/';
      break;
   case OP_LT:
      o << '<';
      break;
   case OP_GT:
      o << '>';
      break;
   case OP_LE:
      o << "<=";
      break;
   case OP_GE:
      o << ">=";
      break;
   case OP_EQ:
      o << "==";
      break;
   case OP_NE:
      o << "!=";
      break;
   case OP_AND:
      o << "&&";
      break;
   case OP_OR:
      o << "||";
      break;
   default:
      o << " ** ?? ** ";
 }
 o << ' ' << *m_right << ')' ; 
}

void InvExpr::evalTo(CodeCtx &ctx, const string &target) {
  m_expr->evalTo(ctx,target);
  emit ( ctx, "inv %s\t\t\t; !(expr)\n", target.c_str() );
}

void BinaryExpr::evalTo(CodeCtx &ctx, const string &target) {
  if (m_oper != OP_AND && m_oper != OP_OR) {
    m_left->evalTo(ctx, "_r0"); 
    emit ( ctx, "push _r0\t\t\t; left hand result\n" ); 
    m_right->evalTo(ctx, "_r1");  
    emit ( ctx, "pop _r0\t\t\t; expr left\n" );
    switch (m_oper) {
        case OP_ADD:
          emit(ctx, "add _r0 _r1 %s\t\t\t; + \n", target.c_str());
          break;
        case OP_SUB:
          emit(ctx, "sub _r0 _r1 %s\t\t\t; - \n", target.c_str());
          break;
        case OP_MULT:
          emit(ctx, "mul _r0 _r1 %s\t\t\t; * \n", target.c_str());
          break;
        case OP_DIV:
          emit(ctx, "div _r0 _r1 %s\t\t\t; * \n", target.c_str());
          break;
        case OP_LT:
          emit(ctx, "low _r0 _r1 %s\t\t\t; < \n", target.c_str());
          break;
        case OP_GT:
          emit(ctx, "high _r0 _r1 %s\t\t\t; > \n", target.c_str());
          break;
        case OP_LE:
          emit(ctx, "lte _r0 _r1 %s\t\t\t; <= \n", target.c_str());
          break;
        case OP_GE:
          emit(ctx, "gte _r0 _r1 %s\t\t\t; >= \n", target.c_str());
          break;
        case OP_EQ:
          emit(ctx, "cmp _r0 _r1 %s\t\t\t; == \n", target.c_str());
          break;
        case OP_NE:
          emit(ctx, "cmp _r0 _r1 %s\t\t\t; ne pre \n", target.c_str() );
          emit(ctx, "inv %s\t\t\t; != \n", target.c_str() );
          break;
        default:
          CTX->error ( "unimplemented binary operator." );
    }
  } else {
    // and,or
    int end = CTX->NextLabel();
    m_left->evalTo(ctx,target);
    emit ( ctx, "tob %s\t\t\t;\n", target.c_str() );
    emit ( ctx, "cmpjump %s %d %d 0\t\t\t;left?\n", target.c_str(), m_oper == OP_AND ? 0 : 1, end );
    m_right->evalTo(ctx,target);
    emit ( ctx, "label %d; end logic oper\n", end );
  }
}

void FuncExpr::stream(ostream &o) const {
  
  o << m_name << '(';
  for (FunctionCallArgList::const_iterator itr = m_args->begin();
       itr < m_args->end();
       ++itr ) {
      o << **itr << ',';
  }
  o << ')';

}
FuncExpr::~FuncExpr() {
  for (FunctionCallArgList::iterator itr = m_args->begin();
       itr < m_args->end();
       ++itr) {
       delete *itr;
  }
  delete m_args;
}

void FuncExpr::genCall(CodeCtx &ctx) {
   
   // push args in reverse order
   for (FunctionCallArgList::reverse_iterator itr = m_args->rbegin();
        itr < m_args->rend();
        ++itr) {
        (*itr)->evalTo(ctx, "_r0");
        emit ( ctx, "push _r0\t\t\t; function argument\n" );
   }
   int funcLabel = CTX->GetFunctionLoc(m_name);
   emit ( ctx, "jump %d 1\t\t\t; call %s\n", funcLabel, m_name.c_str() );
  
}

void FuncExpr::genCode(CodeCtx &ctx) {
 DEBUG ( cout << *this << endl );
 genCall(ctx); 
 int type = CTX->GetFunctionRet(m_name);
 if (type) emit ( ctx, "pop\t\t\t; ignoring func ret value.\n" );
}

void FuncExpr::evalTo(CodeCtx &ctx, const string &target) {
   genCall(ctx);
   int type = CTX->GetFunctionRet(m_name);
   if (type) 
      emit ( ctx, "pop %s\t\t\t; func result\n", target.c_str() );
   else {
      CTX->warning ( "function (%s) does not return a value.", m_name.c_str() );
      emit ( ctx, "move %s 0\t\t\t; void func used in expression\n", target.c_str() );
   }
}

void ThreadExpr::genCode(CodeCtx &ctx) {
  DEBUG ( cout << "thread (" << m_name << " ) " );
  evalTo(ctx,"_r0"); // ignore tId when used as a raw statement.
}

void ThreadExpr::evalTo(CodeCtx &ctx, const string &target) {
   int l=CTX->GetFunctionLoc(m_name);
   emit ( ctx, "thread %d %s\t\t\t; start thread %s\n", l, target.c_str(), m_name.c_str() );
}

void RunningExpr::genCode(CodeCtx &ctx) {
  DEBUG ( cout << "is_running ( " << m_var << " ) ");
  evalTo(ctx,"_r0");
}

void RunningExpr::evalTo(CodeCtx &ctx, const string &target) {
  string loc = CTX->FindVarLoc(m_var);
  emit( ctx, "alive %s %s\t\t\t; is_running %s\n", loc.c_str(), target.c_str(), m_var.c_str() );
}
 

