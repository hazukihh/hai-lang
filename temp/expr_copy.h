#ifndef HAILANG_EXPR_COPY_H
#define HAILANG_EXPR_COPY_H
#pragma once
#include <memory>

#include "../src/tokens_def.h"
#include "../src/arena.h"


inline uint8_t g_buffer[1024*4];
inline Arena g_arena {
  .size_ = 0,
  .cap_ = sizeof(g_buffer),
  .pool_ = g_buffer
};


G_WATCHER(expr_copy);

struct Expr
{
  G_OPERATOR_NEW_WATCHER_BY(expr_copy)

  using Ptr = Expr*;

  Token atom {.type = ETokenType::ETK_None};
  // TODO: store by left+right or vec![] ? which better ?
  Ptr left = nullptr;
  Ptr right = nullptr;

  void print(std::string (*get_value)(const Token& t),int depth = 0) const;
};

struct Lexer;
/**
 * @brief  Pratt parser for parsing expressions.
 *    primary ::= Identifier | IntLit | FlLit | StrLit
 *    unary ::= + - ! etc. (special "()" )
 *    binary ::= + - * / = etc.
 *    expr ::= unary expr | ( expr )
 *           | expr binary expr | primary
 */
Expr::Ptr parse_expression(Lexer *lexer, int rbp = 0);


#endif //HAILANG_EXPR_COPY_H
