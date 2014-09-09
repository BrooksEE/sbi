
#include "ast.h"

#include <map>

#include <stdio.h>
#include <stdarg.h>
#include <typeinfo>

using namespace std;


#define RESERVED_GLOBALS 2 // for ms, FRAME_COUNT
#define GLOBALS_MAX (64-RESERVED_GLOBALS) // 64 from sbi defs

typedef map<string,string> VarCtx;
typedef vector<VarCtx*> VarScope;

typedef map<string,int> LabelNums;

class CtxImpl {
 public:
    bool debug;

    VarScope varScope;
    uint8_t regStack; // can go from 0-15

    LabelNums labels; 
    int nextLabel;

    CtxImpl() :
        regStack(2), // start at 2 to reserve _r0 and _r1 for expressions.
        nextLabel(0) {
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
       if (varScope.size()) // if 0 then we don't use regs
          regStack -= varScope.size();
       delete back;
    }
    VarCtx* CurVarCtx() { return varScope.back(); } 
    
    void AddVar(string &var) {
       VarCtx* ctx = CurVarCtx();
       // if has already error
       if (ctx->count(var)>0) {
            error ( "variable already defined." );
       }
       
       // add var to context
       char rbuf[5];
       VarCtx* glbl = varScope.front();
       if (glbl->size() < GLOBALS_MAX || varScope.size()==1) {
           // in this case, go ahead and just use a free global variable
           // instead of registers.
           if (glbl->size() > GLOBALS_MAX-1) {
              error ( "global variable overflow." );
           }
           snprintf(rbuf,5,"_t%d", glbl->size()+RESERVED_GLOBALS);
           (*glbl)[var] = string(rbuf);
       } else {
          // put variable in current scope. 
          if (regStack>15) {
               error ( "Too many local variables." );
          }
          snprintf(rbuf,5,"_r%d",regStack++);
          (*ctx)[var] = string(rbuf);
       }
       
    }

    string FindVarLoc(string &var) {
       VarScope::reverse_iterator itr;
       for (itr=varScope.rbegin();
            itr<varScope.rend();
            ++itr) {
            VarCtx* ctx = *itr;
            if (ctx->count(var)>0) {
                return (*ctx)[var];
            }
       }

       error ( "Undefined variable" );
       return "error";
    }


    // labels
    void AddNameLabel(string &name) {
        if ( labels.count(name)>0) {
            error ( "Label already exists." );
        }
        int nextl = labels.size();
        labels[name] = nextl; 
    }
    int GetNameLabel(string &name) {
        if ( labels.count(name)==0 ) error ( "Label not found." );
        return labels[name];
    }

    int NextLabel() {
        return ++nextLabel + labels.size();
    }

    // error management
    void error(const char* err) {
        cout << err << endl;
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
  for (Globals::iterator itr = m_globals.begin();
       itr < m_globals.end();
       ++itr) {
       delete *itr;
  }
  for (FunctionList::iterator itr = m_functions.begin();
       itr < m_functions.end();
       ++itr) {
       delete *itr;
  }
  for (ThreadList::iterator itr = m_threads.begin();
       itr < m_threads.end();
       ++itr) {
       delete *itr;
  }
  
}

void Program::genCode(CodeCtx &ctx) {
  DEBUG(cout << "Generate a program." << endl);
  
  for (Globals::iterator itr = m_globals.begin();
       itr < m_globals.end();
       ++itr) {
       (*itr)->genCode(ctx);
  }
  // pre compute label names
  for (FunctionList::iterator itr = m_functions.begin();
       itr < m_functions.end();
       ++itr) {
       CTX->AddNameLabel((*itr)->m_name);
  }
  for (ThreadList::iterator itr = m_threads.begin();
       itr < m_threads.end();
       ++itr ) {
       CTX->AddNameLabel((*itr)->m_name);
  }

  // generate a main
  ThreadList::iterator itr = m_threads.begin();
  ++itr; // skip first
  int l;
  char tId[255];
  while (itr != m_threads.end()) {
     // all threads but first have a variable for the threadId
     // start it with the label so that it won't conflict with
     // user variables
     l = CTX->GetNameLabel((*itr)->m_name);
     snprintf ( tId, 255, "%d%s", l, (*itr)->m_name.c_str() );
     string tmp(tId);
     CTX->AddVar(tmp);
     string loc = CTX->FindVarLoc(tmp);
     emit ( ctx, "thread %d %s\t\t\t; Start thread %s\n", l, loc.c_str(), (*itr)->m_name.c_str() );
     ++itr;
  }
  // jump to thread 1
  itr = m_threads.begin();
  l=CTX->GetNameLabel((*itr)->m_name);
  emit ( ctx, "jump %d 1\t\t\t; Jump to thread 1 (%s)\n", l, (*itr)->m_name.c_str() );
  ++itr;
  while (itr != m_threads.end()) {
    int l=CTX->GetNameLabel( (*itr)->m_name );
    snprintf ( tId, 255, "%d%s", l, (*itr)->m_name.c_str() );
    string tmp(tId);
    string loc = CTX->FindVarLoc(tmp);
    emit ( ctx, "wait %s\t\t\t; wait for thread %s to finish\n", loc.c_str(), (*itr)->m_name.c_str() );
    ++itr;
  }
  emit ( ctx, "exit\t\t\t; program finish\n" );

  for (FunctionList::iterator itr = m_functions.begin();
       itr < m_functions.end();
       ++itr ) {
       (*itr)->genCode(ctx);
  }

  for (ThreadList::iterator itr = m_threads.begin();
       itr < m_threads.end();
       ++itr ) {
       (*itr)->genCode(ctx);
  }

}


void VarDec::genCode(CodeCtx &ctx) {
  DEBUG(cout << "var: " << m_name << '=' << *m_value << endl);
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
  for (Stmts::iterator itr = m_stmts.begin();
       itr < m_stmts.end();
       ++itr) {
       (*itr)->genCode(ctx);
  }
  CTX->PopVarCtx();
  DEBUG( cout << '}' );
}

Block::~Block() {
  for (Stmts::iterator itr = m_stmts.begin();
       itr < m_stmts.end();
       ++itr ) delete *itr;
}


void Function::genCode(CodeCtx &ctx) {
  DEBUG ( cout << "function " << m_name << '(' );
  DEBUG ( 
     for ( FunctionArgList::iterator itr = m_args.begin();
           itr < m_args.end();
           ++itr ) cout << *itr << ',';
     cout << ')' );

  int funcStart = CTX->GetNameLabel(m_name);
  emit ( ctx, "label %d\t\t\t; function %s\n", funcStart, m_name.c_str() );
  CTX->PushVarCtx(); 

  for (FunctionArgList::iterator itr = m_args.begin();
       itr < m_args.end();
       ++itr) {
       CTX->AddVar(*itr);
  }
  for (FunctionArgList::reverse_iterator itr = m_args.rbegin();
       itr < m_args.rend();
       ++itr) {
       string loc = CTX->FindVarLoc(*itr);
       emit ( ctx, "pop %s\t\t\t; arg %s\n", loc.c_str(), itr->c_str() ); 
  }
  m_block.genCode(ctx);
  CTX->PopVarCtx();
  // is last statement a return
  bool pushval=true;
  if (m_block.m_stmts.size()) {
     Node* last = m_block.m_stmts.back(); 
     if (typeid(*last) == typeid(ReturnStmt))
        pushval=false;
  }
  if (pushval) {
    emit(ctx, "push 0\t\t\t; no return in code\n" );
    emit(ctx, "ret\t\t\t; return\n" );
  }
}

void Thread::genCode(CodeCtx &ctx) {
 DEBUG ( cout << "thread " << m_name);
 int l=CTX->GetNameLabel(m_name);
 emit ( ctx, "label %d\t\t\t; thread %s\n", l, m_name.c_str() );
 m_block.genCode(ctx);
 emit ( ctx, "ret\t\t\t; end thread %s\n", m_name.c_str() );
}

// statements
void AssignStmt::genCode(CodeCtx &ctx) {
 DEBUG ( cout << m_lval << '=' << *m_val << endl );
 // TODO special lval cases
 
 string loc = CTX->FindVarLoc(m_lval);
 m_val->evalTo(ctx, loc);

}

void FuncCallStmt::genCode(CodeCtx &ctx) {
 DEBUG ( cout << *m_call << endl );
 ((Expr*)m_call)->evalTo(ctx, "_r0");
}
FuncCallStmt::~FuncCallStmt() { delete m_call; }

void WhileStmt::genCode(CodeCtx &ctx) {
 DEBUG ( cout << "While (" << *m_expr << ") " ); 
  int whileStart = CTX->NextLabel();
  int whileEnd = CTX->NextLabel();
  emit ( ctx, "label %d\t\t\t; while\n",whileStart ); 
  m_expr->evalTo(ctx, "_r0");
  emit ( ctx, "cmpjump 0 _r0 %d 0\t\t\t; if 0 jump to end while\n", whileEnd );
  m_block.genCode(ctx);
  emit ( ctx, "jump %d 0\t\t\t; loop\n", whileStart );
  emit ( ctx, "label %d\t\t\t; end while\n", whileEnd );
}

void IfStmt::genCode(CodeCtx &ctx) {
 DEBUG ( cout << "If (" << *m_eval << ") " );
 int ifStart = CTX->NextLabel();
 emit ( ctx, "label %d\t\t\t; if ..\n", ifStart );
 int ifEnd = CTX->NextLabel(); 
 m_eval->evalTo(ctx,"_r0");
 emit ( ctx, "cmpjump _r0 0 %d 0\t\t\t; false -> else\n", ifEnd );
 m_true.genCode(ctx);
 int elseEnd=0; // unused unless else
 if ( m_false.m_stmts.size() ) {
    elseEnd = CTX->NextLabel();
    // still in the if block so jump to the end
    emit ( ctx, "jump %d 0\t\t\t; skip else\n", elseEnd );
 }
 emit ( ctx, "label %d\t\t\t; end if\n", ifEnd );
 if (m_false.m_stmts.size() ) {
    DEBUG ( cout << " else " );
    m_false.genCode(ctx);
    emit ( ctx, "label %d\t\t\t; end else\n", elseEnd );
 }

}

void ReturnStmt::genCode(CodeCtx &ctx) {
 DEBUG ( cout << "return "; 
         if (m_expr) cout << *m_expr; );
 if (m_expr) {
    m_expr->evalTo(ctx,"_r0");
    emit (ctx, "push _r0\t\t\t; return value\n" );
 } else
    emit(ctx,"push 0\t\t\t; empty return\n" );
 emit(ctx,"ret\t\t\t;\n" );
}

void DebugStmt::genCode(CodeCtx &ctx) {
  DEBUG ( cout << "debug ( " << *m_expr << " ) " );
  m_expr->evalTo(ctx, "_r0");
  emit ( ctx, "%s _r0\n", m_call.c_str() );
}

void WaitStmt::genCode(CodeCtx &ctx) {
  DEBUG ( cout << "wait ( " << m_wait << " ) " );
  char tmp[255];
  int l=CTX->GetNameLabel(m_wait);
  snprintf(tmp,255,"%d%s", l, m_wait.c_str() );
  string tId(tmp);
  string loc = CTX->FindVarLoc(tId);
  emit (ctx, "wait %s\t\t\t; wait %s\n", loc.c_str(), m_wait.c_str() );
}

// expressions
void Expr::genCode(CodeCtx &ctx) {
  CTX->error(" ** expression should use eval (compiler error) ** " );
}
void Expr::evalTo(CodeCtx &ctx, string &target) {
  CTX->error(" ** evalTo not implemented ** " );
}

void IntExpr::evalTo(CodeCtx &ctx, string &target) {
   emit ( ctx, "move %s %d;\t\t\tInt Expr\n", target.c_str(), m_val );
}

void VarExpr::evalTo(CodeCtx &ctx, string &target) {
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
   default:
      o << " ** ?? ** ";
 }
 o << ' ' << *m_right << ')' ; 
}

void InvExpr::evalTo(CodeCtx &ctx, string &target) {
  m_expr->evalTo(ctx,target);
  emit ( ctx, "inv %s\t\t\t; !(expr)\n", target.c_str() );
}

void BinaryExpr::evalTo(CodeCtx &ctx, string &target) {
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
}

void FuncExpr::stream(ostream &o) const {
  
  o << m_name << '(';
  for (FunctionCallArgList::const_iterator itr = m_args.begin();
       itr < m_args.end();
       ++itr ) {
      o << **itr << ',';
  }
  o << ')';

}
FuncExpr::~FuncExpr() {
  for (FunctionCallArgList::iterator itr = m_args.begin();
       itr < m_args.end();
       ++itr) {
       delete *itr;
  }
}

void FuncExpr::evalTo(CodeCtx &ctx, string &target) {
   // push args in reverse order
   for (FunctionCallArgList::reverse_iterator itr = m_args.rbegin();
        itr < m_args.rend();
        ++itr) {
        (*itr)->evalTo(ctx, "_r0");
        emit ( ctx, "push _r0\t\t\t; function argument\n" );
   }
   int funcLabel = CTX->GetNameLabel(m_name);
   emit ( ctx, "jump %d 1\t\t\t; call %s\n", funcLabel, m_name.c_str() );
   emit ( ctx, "pop %s\t\t\t; func result\n", target.c_str() );
}


