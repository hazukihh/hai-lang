#pragma once

#include "lexer.h"

#include <cassert>
#include <iostream>
#include <memory>


inline void error(const Token& t,StringView msg) {
  LOG_ERROR("at line{}({}): {}, {}",t.line,t.column,t.to_str(),msg);
  exit(1);
}

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
  G_OPERATOR_NEW_WATCHER
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
    {ETK_Identifier,{prefix_number,nullptr,100}},
    {ETK_IntLit ,{prefix_number,nullptr,100}},
    // Term
    {ETK_Plus  ,  {nullptr,infix_binary,60}},
    {ETK_Minus  , {prefix_minus,infix_binary,60}},
    //Factor
    {ETK_Star  , {nullptr,infix_binary,70}},
    {ETK_Slash  , {nullptr,infix_binary,70}},
    {ETK_LParen ,{prefix_lparen,nullptr,0}},
    {ETK_Assign ,{nullptr,infix_assign,10,1}},
  };

  const auto iter = kRules.find(index);
  if (iter == kRules.end())
    return kRules[ETK_None];
  return iter->second;
}





/*
enum class EPrecedence
{
  PREC_ASSIGNMENT,  // =
  PREC_OR,          // or
  PREC_AND,         // and
  PREC_EQUALITY,    // == !=
  PREC_COMPARISON,  // < > <= >=
  PREC_TERM,        // + -
  PREC_FACTOR,      // * /
  PREC_UNARY,       // ! -
  PREC_CALL,        // . ()
  PREC_PRIMARY
};
*/





static AstNode::Ptr prefix_number(Lexer *, AstNode::Ptr node) {
  return node;
}

static AstNode::Ptr prefix_minus(Lexer *lexer, AstNode::Ptr node)
{
  auto operand = parse_expression(lexer, 150);
  node->right = std::move(operand);
  return node;
}

static AstNode::Ptr prefix_lparen(Lexer *lexer, AstNode::Ptr node)
{
  auto result = parse_expression(lexer, 0);
  const Token t = lexer->next();
  if (t.type != ETK_RParen)
  {
    error(t, "lack ')'" );
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
    error(left->atom,"expect expression");
    return nullptr;
  }
  left = prefix_fn(lexer, std::move(left));

  for (;;) {
    AstNode::Ptr op = std::make_unique<AstNode>(lexer->peek());
    const auto& rule = GetRules(op->atom.type);
    if (rule.lbp == 0 || rbp >= rule.lbp) {
      break;
    }

    lexer->next(); // consume
    const InfixFn infix_fn = rule.infix;
    if (!infix_fn) {
      error(op->atom,"");
      break;
    }

    op->left = std::move(left);
    left = infix_fn(lexer, std::move(op));
  }

  return left;
}
