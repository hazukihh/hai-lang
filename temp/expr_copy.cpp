#include "expr_copy.h"

#include "../src/lexer.h"

void Expr::print(std::string(* get_value)(const Token& t), int depth) const
{
  constexpr int SPACE_SIZE = 4;

  const bool is_punct_type = ETK_PUNCT_START <= this->atom.type && this->atom.type < ETK_PUNCT_END;

  if (right) right->print(get_value,depth + 1);
  else if (is_punct_type) LOG_DEBUG("{:{}}nul","",(depth+1)*SPACE_SIZE);

  LOG_DEBUG("{:{}}{}","",(depth)*SPACE_SIZE,get_value(this->atom));

  if (left) left->print(get_value,depth + 1);
  else if (is_punct_type) LOG_DEBUG("{:{}}nul","",(depth+1)*SPACE_SIZE);
}



/// ==============
/// Internal Decl
/// ==============

/*
0  unary-lparen (
10	= 以及 += -= 等复合赋值，右结合
20	||
30	&&
40	|
50	^
60	&
70	==， !=
80	<， >， <=， >=
90	<<，>>
term100	 +，-
factor110	 *， /， %
unary120	一元前缀（最高）	! ~ + -
                      *（解引用）
                      &（取地址）
postfix / call:
130		.（点）， ->（箭头）	.（成员访问）
140		()， []
150 primary
*/

enum EPrecedence
{
  PREC_ASSIGNS     =10,  // =,+=,-=,...
  PREC_OR          =20,  // ||
  PREC_AND         =30,  // &&
  PREC_BITOR       =40,  // |
  PREC_BITXOR      =50,  // ^
  PREC_BITAND     =60,  // &
  PREC_EQUALITY    =70,  // == !=
  PREC_COMPARISON  =80,  // < > <= >=
  PREC_SHIFT       =90,  // <<,>>
  PREC_TERM        =100, // + -
  PREC_FACTOR      =110, // * / %
  PREC_UNARY       =120, // ! - ~ + (
  PREC_POST        =130, // . -> , () []
  PREC_CALL        =140,
  PREC_PRIMARY     =150
};


using PrefixFn = Expr::Ptr (*)(Lexer *, const Token& node);
using InfixFn  = Expr::Ptr (*)(Lexer *, const Token& node,Expr::Ptr left);

struct Rule
{
  PrefixFn prefix = nullptr;
  InfixFn infix = nullptr;
  int lbp = 0;
  uint8_t is_right = 0;
};
// define Rule Table here
static const Rule& GetRules(ETokenType index);

static Expr::Ptr prefix_number(Lexer *, const Token&);
static Expr::Ptr prefix_minus(Lexer *, const Token&);
static Expr::Ptr prefix_lparen(Lexer *, const Token&);

static Expr::Ptr infix_binary(Lexer *lexer, const Token& node,Expr::Ptr left);
// static Expr::Ptr infix_plus(...)
// static Expr::Ptr infix_minus(...)
// static Expr::Ptr infix_start(...)
// static Expr::Ptr infix_slash(...)
// static Expr::Ptr infix_assign(...)


/// =======
///  Impl
/// =======


const Rule& GetRules(ETokenType index)
{
  static std::unordered_map<int,Rule> kRules = {
    {ETK_None,{}},
    {ETK_LParen ,{prefix_lparen,nullptr,0}},
    {ETK_Assign ,{nullptr,infix_binary,10,1}},
    {ETK_Or,{nullptr,infix_binary,20}},
    {ETK_And,{nullptr,infix_binary,30}},
    {ETK_BitOr,{nullptr,infix_binary,40}},
    {ETK_BitXor,{nullptr,infix_binary,50}},
    {ETK_BitNot,{nullptr,infix_binary,60}},
    // Equality
    {ETK_Eq,{nullptr,infix_binary,70}},
    {ETK_NotEq,{nullptr,infix_binary,70}},
    // Comparison
    {ETK_LessThan,{nullptr,infix_binary,80}},
    {ETK_GreaterThan,{nullptr,infix_binary,80}},
    {ETK_LessEq,{nullptr,infix_binary,80}},
    {ETK_GreaterEq,{nullptr,infix_binary,80}},

    {ETK_LShift,{nullptr,infix_binary,90}},
    {ETK_RShift,{nullptr,infix_binary,90}},
    // Term
    {ETK_Plus  ,  {nullptr,infix_binary,100}},
    {ETK_Minus  , {prefix_minus,infix_binary,100}},
    //Factor
    {ETK_Star  , {nullptr,infix_binary,110}},
    {ETK_Slash  , {nullptr,infix_binary,110}},
    // unary '-': 120
    // call/postfix : 130 140
    // primary ?
    {ETK_Identifier,{prefix_number,nullptr,150}},
    {ETK_IntLit ,{prefix_number,nullptr,150}},
    {ETK_FlLit ,{prefix_number,nullptr,150}},
    {ETK_CharLit ,{prefix_number,nullptr,150}},
    {ETK_StrLit ,{prefix_number,nullptr,150}}
  };

  const auto iter = kRules.find(index);
  if (iter == kRules.end())
    return kRules[ETK_None];
  return iter->second;
}

Expr::Ptr ptr_move(Expr::Ptr& ptr)
{
  Expr::Ptr temp = nullptr;
  std::swap(temp,ptr);
  return temp;
}

#define G_ARENA_NEW(Type) ::new (static_cast<Type*>(g_arena.alloc(sizeof(Type)))) Type

Expr::Ptr prefix_number(Lexer *, const Token& node) {
  return G_ARENA_NEW(Expr)(node);
}

Expr::Ptr prefix_minus(Lexer *lexer, const Token& node)
{
  Expr::Ptr operand = parse_expression(lexer, PREC_UNARY);

  return G_ARENA_NEW(Expr)(
    node,
    nullptr,
    ptr_move(operand)
    );
}

Expr::Ptr prefix_lparen(Lexer *lexer, const Token&)
{
  Expr::Ptr result = parse_expression(lexer, 0);

  if (! lexer->skip(ETK_RParen))
  {
    return nullptr;
  }

  return result;
}


Expr::Ptr infix_binary(Lexer *lexer, const Token& node,Expr::Ptr left)
{
  const auto& rule = GetRules(node.type);
  Expr::Ptr right = parse_expression(lexer, rule.lbp - rule.is_right);

  return G_ARENA_NEW(Expr)(
    node,
    left,
    ptr_move(right)
    );
}



Expr::Ptr parse_expression(Lexer *lexer, int rbp /* = 0 */)
{
  auto left_tok = lexer->next();

  const PrefixFn prefix_fn = GetRules(left_tok.type).prefix;
  if (!prefix_fn) {
    error_at(left_tok,"expect expression");
    return nullptr;
  }
  Expr::Ptr left = prefix_fn(lexer,left_tok);

  for (;;) {
    auto op_tok = lexer->peek();

    const auto& rule = GetRules(op_tok.type);
    if (rule.lbp == 0 || rbp >= rule.lbp) {
      break;
    }

    lexer->consume();
    const InfixFn infix_fn = rule.infix;
    if (!infix_fn) {
      // TODO: what msg is better?
      error_at(op_tok,"expect operator");
      break;
    }

    left = infix_fn(lexer,op_tok,left);
  }

  return left;
}


