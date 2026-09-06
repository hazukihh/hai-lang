#pragma once


#include <string>

#include "expr.h"
#include "type.h"

struct Lexer;
struct Token;

struct Stmt;

// struct Null_Stmt {};
struct Expr_Stmt
{
  Expr* expr = nullptr; // optional
};

struct Ret_Stmt {
  Expr* val = nullptr; // optional
};

// using Compound_Stmt = Block_Stmt;
struct Block_Stmt {
  Stmt* *stmts = nullptr;
  size_t cnt = 0;
};
struct If_Stmt {
  Expr* cond = nullptr;
  Stmt* then; // Block_Stmt
  Stmt* els = nullptr; // optional
};

struct While_Stmt {
  Expr* cond = nullptr;
  Stmt* body; // Block_Stmt
};

struct Break_Stmt
{
  Token tok;
};
struct Continue_Stmt
{
  Token tok;
};

struct Var_Decl_Stmt
{
  // TODO: IdentInfo *name(ref);TypeInfo *type(ref);
  Token name;
  TypeInfo* type = nullptr;
  Expr* init = nullptr; // optional
};

struct Fn_Decl_Stmt
{
  Token name;
  TypeInfo *type = nullptr;
  Stmt* body = nullptr; // BlockStmt,not_null
};


enum ENodeKind
{
  ND_INVALID,
  ND_EXPR_STMT,
  // ND_NULL_STMT,
  ND_RETURN_STMT,
  ND_BLOCK_STMT,
  ND_IF_STMT,
  ND_WHILE_STMT,
  ND_BREAK_STMT,
  ND_CONTINUE_STMT,
  ND_VAR_DECL,
  ND_FN_DECL,
  ND_COUNT
};

using StmtData = std::variant<
  std::monostate,
  Expr_Stmt,
  //Null_Stmt,
  Ret_Stmt,
  Block_Stmt,
  If_Stmt,
  While_Stmt,
  Break_Stmt,
  Continue_Stmt,
  Var_Decl_Stmt,
  Fn_Decl_Stmt
>;
static_assert(std::variant_size_v<StmtData> == ND_COUNT);


struct Stmt
{
  using Ptr = Stmt*;
  StmtData data;


};

void print(const Stmt* self,int depth = 0);
void print(const Stmt& self,int depth = 0);

void ast_print(std::string (*get_value)(const Token& t),int depth = 0);


Stmt::Ptr parse_stmt(Lexer *lexer);
