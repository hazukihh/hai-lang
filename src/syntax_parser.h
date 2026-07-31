#pragma once

#include <memory>
#include <string>

#include "expr.h"


struct Lexer;
struct Token;

enum ENodeKind
{
  ND_INVALID,
  ND_RETURN_STMT,

  ND_EXPR_STMT,
  ND_NULL_STMT,
  ND_BLOCK_STMT,
  ND_IF_STMT,
  ND_VAR_DECL,
  ND_FN_DECL,
};
struct Stmt;
struct Expr;
struct Ret_Stmt {
  Expr val;
};
struct Block_Stmt {
  Stmt* stmts;
};
// using Compose??_Stmt = Block_Stmt;

struct If_Stmt {
  Expr cond;
  Block_Stmt then;
  Block_Stmt els;
};
struct While_Stmt {
  Expr cond;
  Block_Stmt body;
};
// control // struct Break_Continue_Stmt;


struct AstNode {
  using Ptr = std::unique_ptr<AstNode>;

  ENodeKind kind = ENodeKind::ND_INVALID;

  union {
    Expr expr;
  };

  Ptr next = nullptr;


  void print(std::string (*get_value)(const Token& t),int depth = 0) const;
};


/**
 * stmt ::= ret_stmt     =>  "return" expr? ";"
 *        | block_stmt  =>  "{" stmt* "}"
 *        | if_stmt     =>  "if" expr block_stmt ["else" block_stmt | "else" if_stmt]
 *        | expr_stmt / null_stmt  =>  expr? ";"
 *        | var_decl_stmt  => "var" <identifier> [":" [<Type>] | ":=" expr | :: expr]";" // like jai
 *                     or? => "var" <identifier> [":"<Type>] ["="expr]";"  // which better?
 */
inline AstNode::Ptr parse_stmt(Lexer *lexer);
