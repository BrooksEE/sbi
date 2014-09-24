
#ifndef AST_H
#define AST_H

#include <vector>
#include <string>
#include <iostream>

#include <stdint.h>

class Node;
class VarDec;
class Function;
class Expr;


enum Oper {
  OP_ADD,
  OP_SUB,
  OP_MULT,
  OP_DIV,
  OP_LT,
  OP_GT,
  OP_LE,
  OP_GE,
  OP_EQ,
  OP_NE
};

typedef std::vector<VarDec*> Globals;
typedef std::vector<Node*> Stmts;
typedef std::vector<Function*> FunctionList;
typedef std::vector<std::string> FunctionArgList;
typedef std::vector<Expr*> FunctionCallArgList;

class CodeCtx {
  public:
    CodeCtx(bool debug);
    ~CodeCtx();
    void *impl;
};


class Node {
   public:
   virtual ~Node() {};
   virtual void genCode(CodeCtx &);
   void emit(CodeCtx &,const char*, ...);
};

class Expr : public Node {
  public:
   virtual void stream(std::ostream &o) const { o << "** stream not implemented **";  }
   virtual void evalTo (CodeCtx &ctx, const std::string &target );
   void genCode( CodeCtx &ctx );
};


class Program : public Node {
  public:
  Globals *m_globals;
  FunctionList *m_functions;
  Program( Globals *g, FunctionList *f ) : 
    m_globals(g), 
    m_functions(f) {}
  ~Program();

  void genCode(CodeCtx &);
};

class VarDec : public Node {
  public:
  std::string m_name; 
  Expr *m_value;  // must be stored as pointers instead
                  // otherwise copy on assignment loses derived class.
  
  VarDec(char *name, Expr *value) : m_name(name), m_value(value) {}
  VarDec(char* name ) : m_name(name), m_value(NULL) {}
  ~VarDec() { delete m_value; }

  void genCode(CodeCtx &);

};

class Block : public Node {
  public:
  Stmts *m_stmts;
  Block ( Stmts *stmts ) : m_stmts ( stmts ) {}
  Block () : m_stmts ( new Stmts() ) {}
  ~Block();

  void genCode(CodeCtx &);
};

class Function : public Node {
  public:
  int m_rettype;
  std::string m_name;
  FunctionArgList *m_args;
  Block *m_block;
  Function ( int rettype, char* name, FunctionArgList * args, Block* block ) : 
    m_rettype(rettype),
    m_name(name), 
    m_args(args),
    m_block(block) {}
  ~Function() { delete m_args; delete m_block; }

  void genCode(CodeCtx &);
};

// statements
class AssignStmt : public Node {
   public:
   std::string m_lval;
   Expr *m_val;
   AssignStmt ( const char *lval, Expr *val ) :
                m_lval(lval), 
                m_val(val) {}
   ~AssignStmt() { delete m_val; }
  void genCode(CodeCtx &);
};

class FuncExpr;
class FuncCallStmt : public Node {
  public:
  FuncExpr *m_call;
  FuncCallStmt( FuncExpr* call) : m_call(call) {}
  ~FuncCallStmt();
  void genCode(CodeCtx &);
};

class ForStmt : public Node {
  public:
  Node *m_begin;
  Expr *m_cond;
  Node *m_step;
  Block *m_block;
  ForStmt( Node *begin, Expr *cond, Node *step, Block *block ):
    m_begin(begin),
    m_cond(cond),
    m_step(step),
    m_block(block) {}
  ForStmt( Node *begin, Expr *cond, Node *step, Node *stmt ):
    m_begin(begin),
    m_cond(cond),
    m_step(step) {
    m_block = new Block();
    m_block->m_stmts->push_back(stmt);
  }
  ForStmt( Node *begin, Expr *cond, Node *step):
    m_begin(begin),
    m_cond(cond),
    m_step(step),
    m_block(NULL) {}
  ~ForStmt() {
    if (m_begin) delete m_begin;
    if (m_cond) delete m_cond;
    if (m_step) delete m_step;
    if (m_block) delete m_block;
  }

  void genCode(CodeCtx &ctx);
};

class WhileStmt : public Node {
  public:
  Expr* m_expr;
  Block *m_block;
  WhileStmt ( Expr* expr, Block *block ) :
     m_expr(expr),
     m_block(block) {}
  WhileStmt ( Expr* expr, Node *stmt ) :
     m_expr(expr) { 
        m_block = new Block();
        m_block->m_stmts->push_back(stmt); 
     }
  WhileStmt ( Expr* expr ) : 
     m_expr(expr) , m_block(NULL) {}
  ~WhileStmt() { delete m_expr; if (m_block) delete m_block; }
  void genCode(CodeCtx &);
};

class DowhileStmt : public WhileStmt {
    public:
    DowhileStmt ( Block *block, Expr* expr ) : WhileStmt ( expr, block ) {}
    void genCode(CodeCtx &);
};

class IfStmt : public Node {
  public:
  Expr *m_eval;
  Block *m_true;
  Block *m_false;
  IfStmt ( Expr *eval, Block* t ) :
           m_eval(eval),
           m_true(t),
           m_false(NULL) {}
  
  ~IfStmt() { delete m_eval; delete m_true; if (m_false) delete m_false; }
  void genCode(CodeCtx &);
};

class ReturnStmt : public Node {
  public:
  Expr *m_expr;
  ReturnStmt (Expr *expr) : m_expr(expr) {}
  ~ReturnStmt() { delete m_expr; }
  void genCode(CodeCtx &);
};

class DebugStmt : public Node {
  public:
  Expr *m_expr;
  std::string m_call;
  DebugStmt(const char* call, Expr *expr) : m_call(call), m_expr(expr) {} 
  ~DebugStmt() { delete m_expr; }
  void genCode(CodeCtx &);
};

class WaitStmt : public Node {
  public:
  std::string m_wait;
  WaitStmt ( const char* wait ) : m_wait(wait) {}
  void genCode(CodeCtx &);

};

class StopStmt : public Node {
  public:
  std::string m_stop;
  StopStmt ( const char* stop ) : m_stop(stop) {}
  void genCode(CodeCtx &);
};

class ExitStmt : public Node {
  public:
  ExitStmt() {}
  void genCode(CodeCtx &);
};


// expressions
std::ostream& operator << (std::ostream &o, const Expr &e);

class IntExpr : public Expr {
  public:
  int m_val;
  IntExpr(int val) : m_val(val) {}
  void stream(std::ostream &o) const { o << m_val; }
  void evalTo(CodeCtx &, const std::string &);

};

class VarExpr : public Expr {
  public:
  std::string m_var;
  VarExpr(const char* var) : m_var(var) {}
  void stream(std::ostream &o) const { o << m_var; }
  void evalTo(CodeCtx &, const std::string &);

};

class BinaryExpr : public Expr {
  public:
  Expr *m_left;
  Oper m_oper;
  Expr *m_right;
  BinaryExpr ( Expr *l, Oper oper, Expr *r ) :
               m_left(l),
               m_oper(oper),
               m_right(r) {}
  ~BinaryExpr() { delete m_left; delete m_right; }
  void stream(std::ostream &o) const;
  void evalTo(CodeCtx &, const std::string &);
};

class InvExpr : public Expr {
  public:
  Expr *m_expr;
  InvExpr ( Expr *e ) : m_expr (e) {}
  ~InvExpr() { delete m_expr; }
  void stream ( std::ostream &o) const { o << "!(" << *m_expr << ')'; }
  void evalTo(CodeCtx &, const std::string &);
};

class FuncExpr : public Expr {
  public:
  std::string m_name;
  FunctionCallArgList *m_args;
  FuncExpr ( const char* name, FunctionCallArgList *args) :
             m_name(name),
             m_args(args) {}
  ~FuncExpr();
  void stream ( std::ostream &o) const ;
  void evalTo(CodeCtx &, const std::string &);
};

class UserfuncStmt : public FuncExpr {
  public:
  unsigned int m_ncall;
  std::string m_evalTo;
  UserfuncStmt ( unsigned int n, FunctionCallArgList* args ) : FuncExpr("user", args), m_ncall(n) {}
  void genCode(CodeCtx &);
  void evalTo(CodeCtx &, const std::string &);
};

class ThreadExpr : public Expr {
  public:
  std::string m_name;
  ThreadExpr(const char* name) : m_name(name) {}
  void stream ( std::ostream &o) const { o << "thread (" << m_name << ")"; }
  void genCode(CodeCtx &);
  void evalTo(CodeCtx &, const std::string &);
};

class RunningExpr : public Expr {
  public:
  std::string m_var;
  RunningExpr( const char* var) : m_var(var) {}
  void stream( std::ostream &o) const { o << "is_running (" << m_var << ")"; }
  void genCode(CodeCtx &);
  void evalTo(CodeCtx &, const std::string &);
};

#endif

