#pragma once

#include "lexer.h"

#include <cassert>
#include <iostream>
#include <memory>



// TODO: Arena
#if defined(DEBUG) || defined(_DEBUG)
inline size_t g_allocted_size = 0;
#define G_OPERATOR_NEW_WATCHER\
  void* operator new(size_t size){\
    g_allocted_size += size;\
    return ::operator new(size);\
  }\
  void operator delete(void* ptr,size_t size) {\
    g_allocted_size -= size;\
    return ::operator delete(ptr);\
  }
#endif

struct AstNode {
  using Ptr = std::unique_ptr<AstNode>;

  Token atom {.type = ETokenType::ETK_None};
  // TODO: store by left+right or vec![] ? which better ?
  Ptr left = nullptr;
  Ptr right = nullptr;

#if defined(DEBUG) || defined(_DEBUG)
  // G_OPERATOR_NEW_WATCHER
  void* operator new(size_t size){\
    g_allocted_size += size;\
    return ::operator new(size);\
  }\
  void operator delete(void* ptr,size_t size) {\
    g_allocted_size -= size;\
    return ::operator delete(ptr);\
  }
#endif

  void print(std::string (*get_value)(const Token& t),int depth = 0) const
  {
    auto print_nul = [](int depth)
    {
      for (int i = 0; i < depth; ++i)
        std::cout << "    ";
      std::cout << "nul\n";
    };

    bool is_punct_type = ETK_PUNCT_START <= this->atom.type && this->atom.type < ETK_PUNCT_END;

    if (right) right->print(get_value,depth + 1);
    else if (is_punct_type) print_nul(depth+1);

    for (int i = 0; i < depth; ++i)
      std::cout << "    ";

    std::cout << get_value(this->atom)<< "\n";

    if (left) left->print(get_value,depth + 1);
    else if (is_punct_type) print_nul(depth+1);
  }
};

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
postfix:
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
  PREC_BITXAND     =60,  // &
  PREC_EQUALITY    =70,  // == !=
  PREC_COMPARISON  =80,  // < > <= >=
  PREC_SHIFT       =90,  // <<,>>
  PREC_TERM        =100, // + -
  PREC_FACTOR      =110, // * / %
  PREC_UNARY       =120, // ! - ~ +
  PREC_POST        =130, // . -> , () []
  PREC_CALL        =140,
  PREC_PRIMARY     =150
};



using PrefixFn = AstNode::Ptr (*)(Lexer *, AstNode::Ptr);
using InfixFn = AstNode::Ptr (*)(Lexer *, AstNode::Ptr);

static AstNode::Ptr parse_expression(Lexer *lexer, int rbp);

static AstNode::Ptr prefix_number(Lexer *, AstNode::Ptr);
static AstNode::Ptr prefix_minus(Lexer *, AstNode::Ptr);
static AstNode::Ptr prefix_lparen(Lexer *, AstNode::Ptr);

static AstNode::Ptr infix_binary(Lexer *lexer, AstNode::Ptr node);
// static AstNode::Ptr infix_plus(Lexer *, AstNode::Ptr);
// static AstNode::Ptr infix_minus(Lexer *, AstNode::Ptr);
// static AstNode::Ptr infix_start(Lexer *, AstNode::Ptr);
// static AstNode::Ptr infix_slash(Lexer *, AstNode::Ptr);
static AstNode::Ptr infix_assign(Lexer *lexer, AstNode::Ptr node);

struct Rule
{
  PrefixFn prefix = nullptr;
  InfixFn infix = nullptr;
  int lbp = 0;
  uint8_t is_right = 0;
};


inline const Rule& GetRules(ETokenType index)
{
  static std::unordered_map<int,Rule> kRules = {
    {ETK_None,{}},
    {ETK_LParen ,{prefix_lparen,nullptr,0}},
    {ETK_Assign ,{nullptr,infix_assign,10,1}},
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



static AstNode::Ptr prefix_number(Lexer *, AstNode::Ptr node) {
  return node;
}

static AstNode::Ptr prefix_minus(Lexer *lexer, AstNode::Ptr node)
{
  auto operand = parse_expression(lexer, PREC_UNARY);
  node->right = std::move(operand);
  return node;
}

static AstNode::Ptr prefix_lparen(Lexer *lexer, AstNode::Ptr node)
{
  auto result = parse_expression(lexer, 0);

  if (! lexer->skip(ETK_RParen))
  {
    return nullptr;
  }

  return result;
}


static AstNode::Ptr infix_binary(Lexer *lexer, AstNode::Ptr node)
{
  const auto& rule = GetRules(node->atom.type);
  auto right = parse_expression(lexer, rule.lbp - rule.is_right);
  node->right = std::move(right);
  return node;
}

static AstNode::Ptr infix_assign(Lexer *lexer, AstNode::Ptr node)
{
  const auto& rule = GetRules(node->atom.type);
  auto right = parse_expression(lexer, rule.lbp - rule.is_right);
  node->right = std::move(right);
  return node;
}


/**
 * @brief  Pratt parser for parsing expressions.
 */
AstNode::Ptr parse_expression(Lexer *lexer, int rbp)
{
  auto left = std::make_unique<AstNode>(lexer->next());

  const PrefixFn prefix_fn = GetRules(left->atom.type).prefix;
  if (!prefix_fn) {
    error_at(left->atom,"expect expression");
    return nullptr;
  }
  left = prefix_fn(lexer, std::move(left));

  for (;;) {
    AstNode::Ptr op = std::make_unique<AstNode>(lexer->peek());
    const auto& rule = GetRules(op->atom.type);
    if (rule.lbp == 0 || rbp >= rule.lbp) {
      break;
    }

    lexer->consume();
    const InfixFn infix_fn = rule.infix;
    if (!infix_fn) {
      // TODO: what msg is better?
      error_at(op->atom,"expect operator");
      break;
    }

    op->left = std::move(left);
    left = infix_fn(lexer, std::move(op));
  }

  return left;
}

inline AstNode::Ptr test_parse_statement(Lexer *lexer)
{
  auto left = parse_expression(lexer,0);
  lexer->skip(ETK_Semi);
  return left;
}
