#pragma once

#include <memory>
#include <string>

#include "expr.h"


struct Lexer;
struct Token;

enum ENodeKind
{
  ND_INVALID,
  ND_EXPR_STMT,
  ND_NULL_STMT,
  ND_RETURN_STMT,
  ND_BLOCK_STMT,
};


// TODO: struct AstNode
struct AstNode {
  using Ptr = std::unique_ptr<AstNode>;

  ENodeKind kind = ENodeKind::ND_INVALID;

  // union {
  //   Expr expr;
  // };

  Ptr next = nullptr;


  void print(std::string (*get_value)(const Token& t),int depth = 0) const;
};


/**
 * stmt ::= ret_stmt     =>  "return" expr? ";"
 *        | block_stmt  =>  "{" stmt* "}"
 *        | if_stmt     =>  "if" expr block_stmt ["else" block_stmt | "else" if_stmt]
 *        | expr_stmt   =>  expr? ";"
 */
inline AstNode::Ptr parse_stmt(Lexer *lexer);
