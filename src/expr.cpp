#include "expr.h"

#include "lexer.h"

void Expr::print(std::string(* get_value)(const Token& t), int depth) const
{
  constexpr int SPACE_SIZE = 4;

  const bool is_punct_type = ETK_PUNCT_START <= this->atom.type && this->atom.type < ETK_PUNCT_END;


  if (right)
  {
    if (this->atom.type == ETK_LParen)
    {
      Expr::Ptr p = right;
      int cnt = 1;
      while (p)
      {
        LOG_DEBUG("{:{}}--arg{}--","",(depth+1)*SPACE_SIZE,cnt);
        cnt += 1;
        p->print(get_value,depth + 1);
        p = p->next;
      }
    }
    else
    {
      right->print(get_value,depth + 1);
    }
  }
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

enum EPrecedence : uint8_t
{
  // 0 l: EOF ; )
  // ( r: 0
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
  PREC_UNARY       =120, // ! - ~ + unref-* addressof&
  PREC_CALL        =130, // ()
  PREC_POST        =140, // . -> []
  PREC_PRIMARY     =150
};


using PrefixFn = Expr::Ptr (*)(Lexer *, const Token& self);
using InfixFn  = Expr::Ptr (*)(Lexer *, const Token& self,Expr::Ptr left);

struct Rule
{
  PrefixFn prefix = nullptr;
  InfixFn infix = nullptr;
  uint8_t lbp = 0;
  uint8_t is_right = 0;
};
// define Rule Table here
static const Rule& GetRules(ETokenType index);

static Expr::Ptr prefix_primary(Lexer *, const Token& self);
static Expr::Ptr prefix_unary_op(Lexer *, const Token& self);
static Expr::Ptr prefix_lparen(Lexer *, const Token&);

static Expr::Ptr infix_binary_op(Lexer *lexer, const Token& self,Expr::Ptr left);


static Expr::Ptr infix_call(Lexer *lexer,const Token& self,Expr::Ptr left);
static Expr::Ptr infix_dot(Lexer *lexer,const Token& self,Expr::Ptr left);

static Expr::Ptr infix_arr(Lexer *lexer,const Token& self,Expr::Ptr left);

/// =======
///  Impl
/// =======


const Rule& GetRules(ETokenType index)
{
  // TODO: flat_map?
  static std::unordered_map<int,Rule> kRules = {
    {ETK_None,{}},
    {ETK_Assign ,{nullptr, infix_binary_op,10,1}},
    {ETK_Or,{nullptr,infix_binary_op,20}},
    {ETK_And,{nullptr,infix_binary_op,30}},
    {ETK_BitOr,{nullptr,infix_binary_op,40}},
    {ETK_BitXor,{nullptr,infix_binary_op,50}},
    {ETK_BitAnd,{nullptr,infix_binary_op,60}},
    // Equality
    {ETK_Eq,{nullptr,infix_binary_op,70}},
    {ETK_NotEq,{nullptr,infix_binary_op,70}},
    // Comparison
    {ETK_LessThan,{nullptr,infix_binary_op,80}},
    {ETK_GreaterThan,{nullptr,infix_binary_op,80}},
    {ETK_LessEq,{nullptr,infix_binary_op,80}},
    {ETK_GreaterEq,{nullptr,infix_binary_op,80}},

    {ETK_LShift,{nullptr,infix_binary_op,90}},
    {ETK_RShift,{nullptr,infix_binary_op,90}},
    // Term
    {ETK_Plus  ,  {nullptr,infix_binary_op,100}},
    {ETK_Minus  , {prefix_unary_op,infix_binary_op,100}}, // lbp.infix
    //Factor
    {ETK_Star  , {nullptr,infix_binary_op,110}},
    {ETK_Slash  , {nullptr,infix_binary_op,110}},
    {ETK_Mod  , {nullptr,infix_binary_op,110}},
    // unary : 120
    {ETK_Not  , {prefix_unary_op,nullptr,120}},
    {ETK_BitNot  , {prefix_unary_op,nullptr,120}},
    // call : 130
    {ETK_LParen ,{prefix_lparen,infix_call,130}}, // lbp.infix
    // postfix : 140
    {ETK_Dot ,{nullptr,infix_dot,140}},
    {ETK_LBracket ,{nullptr,infix_arr,140}},
    // primary ?
    {ETK_Identifier,{prefix_primary,nullptr,150}},
    {ETK_IntLit ,{prefix_primary,nullptr,150}},
    {ETK_FlLit ,{prefix_primary,nullptr,150}},
    {ETK_CharLit ,{prefix_primary,nullptr,150}},
    {ETK_StrLit ,{prefix_primary,nullptr,150}}
  };

  const auto iter = kRules.find(index);
  if (iter == kRules.end())
    return kRules[ETK_None];
  return iter->second;
}
/// ============
/// Utils Start
/// ============
#if EXPR_VERSION == 1

#define MAKE_PTR(Type) std::make_unique<Type>
#define MOVE(ptr) std::move(ptr)

#else
Expr::Ptr ptr_move(Expr::Ptr& ptr)
{
  Expr::Ptr temp = nullptr;
  std::swap(temp,ptr);
  return temp;
}
#define MOVE(ptr) ptr_move(ptr)
#define MAKE_PTR(Type) ::new (static_cast<Type*>(g_arena.alloc(sizeof(Type)))) Type

#endif
/// ============
/// Utils End
/// ============


Expr::Ptr prefix_primary(Lexer *, const Token& self) {
  return MAKE_PTR(Expr)(self);
}

Expr::Ptr prefix_unary_op(Lexer *lexer, const Token& self)
{
  Expr::Ptr operand = parse_expression(lexer, PREC_UNARY);

  return MAKE_PTR(Expr)({
    .atom = self,
    .left = nullptr,
    .right = MOVE(operand)
  });
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


Expr::Ptr infix_binary_op(Lexer *lexer, const Token& self,Expr::Ptr left)
{
  const auto& rule = GetRules(self.type);
  Expr::Ptr right = parse_expression(lexer, rule.lbp - rule.is_right);

  return MAKE_PTR(Expr)({
    .atom = self,
    .left = MOVE(left),
    .right = MOVE(right)
  });
}

/**
 * @note
 * self->right
 * : arg1
 *    |->next arg2 -next-> arg3 -next-> ...
 */
Expr::Ptr infix_call(Lexer* lexer, const Token& self, Expr::Ptr left)
{
  Expr::Ptr result = MAKE_PTR(Expr)(self,MOVE(left));
  // call with args
  if (lexer->peek().type != ETK_RParen)
  {
    Expr::Ptr &p = result->right;
    for (;;) {
      p = parse_expression(lexer);
      p = p->next;

      if (lexer->peek().type == ETK_Comma)
      {
        lexer->consume();
      }
      else break;
    }
  }
  lexer->skip(ETK_RParen);


  return result;
}

Expr::Ptr infix_dot(Lexer* lexer, const Token& self, Expr::Ptr left)
{
  Expr::Ptr right = MAKE_PTR(Expr)(lexer->expect(ETK_Identifier));

  return MAKE_PTR(Expr)({
    .atom = self,
    .left = MOVE(left),
    .right = MOVE(right)
  });
}


Expr::Ptr infix_arr(Lexer* lexer, const Token& self, Expr::Ptr left)
{
  Expr::Ptr right = parse_expression(lexer);
  lexer->skip(ETK_RBracket);
  return MAKE_PTR(Expr)(
    self,
    MOVE(left),
    MOVE(right)
  );
}

Expr::Ptr parse_expression(Lexer *lexer, uint8_t rbp /* = 0 */)
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
      error_at(op_tok,"expect binary operator or postfix op or ';'");
      break;
    }

    left = infix_fn(lexer,op_tok,MOVE(left));
  }

  return left;
}


