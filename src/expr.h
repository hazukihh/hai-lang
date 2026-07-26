#ifndef HAILANG_EXPR_H
#define HAILANG_EXPR_H
#pragma once
#include <memory>

#include "tokens_def.h"
#include "arena.h"
#include "common/Log.h"

/**
 * expr_version_1 : unique_ptr + default new
 * expr_version_2 : raw_ptr + arena
 */
#define EXPR_VERSION 2

#if EXPR_VERSION == 2
inline uint8_t g_buffer[1024*4];
inline Arena g_arena {
  .size_ = 0,
  .cap_ = sizeof(g_buffer),
  .pool_ = g_buffer
};
#endif


G_WATCHER(expr);

struct Expr
{
  // G_OPERATOR_NEW_WATCHER_BY(expr)
  void* operator new(size_t size)
  {
    g_expr_allocted_size += size;
    return ::operator new(size);
  }

  void operator delete(void* ptr, size_t size)
  {
    g_expr_allocted_size -= size;
    LOG_DEBUG("delete {}",static_cast<Expr*>(ptr)->atom.to_str());
    return ::operator delete(ptr);
  }

#if EXPR_VERSION == 1
  using Ptr = std::unique_ptr<Expr>;
#else
  using Ptr = Expr*;
#endif

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


#endif //HAILANG_EXPR_H
