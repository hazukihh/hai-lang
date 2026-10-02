#ifndef HAILANG_EXPR_H
#define HAILANG_EXPR_H
#pragma once
#include <memory>
#include <variant>

#include "tokens_def.h"
#include "arena.h"
#include "type.h"
#include "common/Log.h"


inline Arena g_arena;


struct Expr;

// TODO:
//  SourceLocation Loc; // 位置 原始文本通过 SourceManager 获取
//  LineNumberManager

//TODO: IdentRefExpr
struct IdentExpr
{
  IdentInfo *ident = nullptr;
};
// struct LiteralExpr
// {
//   union
//   {
//     uint64_t integer = 0;
//     double fl;
//     uint8_t ch;
//     const uint8_t* str;
//   };
// };
// TODO: Lit { tag; val; suffix;}
struct IntLitExpr
{
  // TODO: BigInt
  uint64_t val = 0;
  // suffix?
};
struct FloatLitExpr
{
  // TODO: BigFl
  long double val = 0;
  // suffix?
};

struct CharLitExpr
{
  uint8_t val = 0;
  uint32_t codepoint = 0; // unicode; max: 0x10FFFF (1,114,111)
};
struct StrLitExpr
{
  // const char* val = nullptr;

  StringView val;

  // suffix?
};

struct UnaryExpr
{
  Expr* operand = nullptr;
};
struct BinaryExpr
{
  Expr* lhs = nullptr;
  Expr* rhs = nullptr;
};

struct CallExpr
{
  Expr* callee = nullptr;
  Expr* *params = nullptr;
  uint32_t params_cnt = 0;
};
struct MemberExpr
{
  Expr* base = nullptr;
  Expr* member = nullptr;
};
struct ArrSubExpr
{
  Expr* base = nullptr;
  Expr* index = nullptr;
};

// union
// {
//   IdentExpr ident;
//   LiteralExpr literal;
//   UnaryOpExpr unary_op;
//   BinaryOpExpr binary_op;
//   CallExpr call_op;
//   MemberExpr mem_op;
//   ArrSubExpr arr_sub_op;
// };

enum EExprTag {
  Expr_Invalid,
  Expr_Ident,
  Expr_IntLit,
  Expr_FloatLit,
  Expr_CharLit,
  Expr_StrLit,
  Expr_Unary,
  Expr_Binary,
  Expr_Call,
  Expr_Member,
  Expr_ArrSub,
  Expr_Count
};
using ExprData = std::variant<
  std::monostate,
  IdentExpr,
  IntLitExpr,
  FloatLitExpr,
  CharLitExpr,
  StrLitExpr,
  UnaryExpr,
  BinaryExpr,
  CallExpr,
  MemberExpr,
  ArrSubExpr
>;
static_assert(std::variant_size_v<ExprData> ==  Expr_Count);

struct Expr
{
#if EXPR_VERSION == 1
  using Ptr = std::unique_ptr<Expr>;
#else
  using Ptr = Expr*;
#endif

  Token atom {.type = ETokenType::ETK_None};

  ExprData data;
  // TODO:
  bool is_error = false;

};

void expr_print(const Expr& self,std::string (*get_value)(const Token& t),int depth = 0);
void expr_print(const Expr* self,std::string (*get_value)(const Token& t),int depth = 0);



struct Lexer;

/**
 * @brief  Pratt parser for parsing expressions.
 *    primary ::= Identifier | IntLit | FlLit | StrLit
 *    unary ::= + - ! etc. (special "()" )
 *    binary ::= + - * / = etc.
 *    expr ::= unary expr | ( expr )
 *           | expr binary expr | primary
 */
Expr::Ptr parse_expression(Lexer *lexer, uint8_t rbp = 0);


#endif //HAILANG_EXPR_H
