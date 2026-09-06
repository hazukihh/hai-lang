#include "expr.h"

#include "lexer.h"
#include "common/variant_match.h"

void expr_print(const Expr& self,std::string(* get_value)(const Token& t), int depth)
{
  constexpr int SPACE_SIZE = 4;
  // print right
  MATCH(self.data,
    [=](const CallExpr& node){
      if (node.params ==nullptr || node.params_cnt == 0)
      {
        LOG_DEBUG("{:{}}nul","",(depth+1)*SPACE_SIZE);
      }
      else for (uint32_t i = 0; i < node.params_cnt; ++i)
      {
        if (node.params[i]) {
          LOG_DEBUG("{:{}}--arg{}--","",(depth + 1) * SPACE_SIZE, i + 1);
          expr_print(node.params[i],get_value, depth + 1);
        }
      }
    },
    [=](const UnaryExpr& node){
      expr_print(node.operand,get_value,depth+1);
    },
    [=](const BinaryExpr& node){
      expr_print(node.rhs,get_value,depth+1);
    },
    [=](const MemberExpr& node){
      expr_print(node.member,get_value,depth+1);
    },
    [=](const ArrSubExpr& node){
      expr_print(node.index,get_value,depth+1);
    },
    [](const auto& _){}
  );


  // print self
  LOG_DEBUG("{:{}}{}","",depth*SPACE_SIZE,get_value(self.atom));

  // print left
  MATCH(self.data,
    [=](const CallExpr& node) {
      expr_print(node.callee,get_value,depth+1);
    },
    [=](const BinaryExpr& node) {
      expr_print(node.lhs,get_value,depth+1);
    },
    [=](const MemberExpr& node) {
      expr_print(node.base,get_value,depth+1);
    },
    [=](const ArrSubExpr& node) {
      expr_print(node.base,get_value,depth+1);
    },
    [=](const UnaryExpr& ) {
      LOG_DEBUG("{:{}}nul","",(depth+1)*SPACE_SIZE);
    },
    [](const auto& _){}
  );
}

void expr_print(const Expr* self,std::string (*get_value)(const Token& t),int depth /*= 0*/)
{
  if (!self)
  {
    constexpr int SPACE_SIZE = 4;
    LOG_DEBUG("{:{}}null-expr", "", depth*SPACE_SIZE);
    return;
  }
  expr_print(*self,get_value,depth);
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
  PREC_UNARY       =120, // ! - ~ address-of-& unref-* +
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
  static phmap::flat_hash_map<int,Rule> kRules = {
    {ETK_None,{}}, // invalid or default
    {ETK_Assign ,{nullptr, infix_binary_op,10,1}},
    {ETK_Or,{nullptr,infix_binary_op,20}},
    {ETK_And,{nullptr,infix_binary_op,30}},
    {ETK_BitOr,{nullptr,infix_binary_op,40}},
    {ETK_BitXor,{nullptr,infix_binary_op,50}},
    {ETK_BitAnd,{prefix_unary_op,infix_binary_op,60}},
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
  switch (self.type) {
  case ETK_Identifier: {
    return MAKE_PTR(Expr)(Expr{
      .atom = self,
      .data = IdentExpr{}
    });
  }break;
  case ETK_IntLit: {
    IntLitExpr int_lit;
    int_lit.val = parse_number<uint64_t>(self.text);

    return MAKE_PTR(Expr)(Expr{
      .atom = self,
      .data = int_lit
    });
  }break;
  case ETK_FlLit: {
    FloatLitExpr fl_lit;
    fl_lit.val = parse_number<long double>(self.text);

    return MAKE_PTR(Expr)(Expr{
      .atom = self,
      .data = fl_lit
    });
  }break;
  case ETK_CharLit: {
    CharLitExpr lit;
    // TODO: parse charlit
    // lit.ch = parse_ch(self.text);
    return MAKE_PTR(Expr)(Expr{
      .atom = self,
      .data = lit
    });
  }break;
  case ETK_StrLit: {
    StrLitExpr str_lit;
    // TODO: Symbol Table / String Pool
    // lit.str = parse_str(self.text);
    return MAKE_PTR(Expr)(Expr{
      .atom = self,
      .data = str_lit
    });
  }break;
  default: break;
  }
  HAI_ASSERT(false && "unreachable");
  return nullptr;
}

Expr::Ptr prefix_unary_op(Lexer *lexer, const Token& self)
{
  Expr::Ptr operand = parse_expression(lexer, PREC_UNARY);

  return MAKE_PTR(Expr)(Expr{
    .atom = self,
    .data = UnaryExpr{
      .operand = MOVE(operand)
    }
  });
}

Expr::Ptr prefix_lparen(Lexer *lexer, const Token&)
{
  Expr::Ptr result = parse_expression(lexer, 0);

  lexer->expect(ETK_RParen);

  return result;
}


Expr::Ptr infix_binary_op(Lexer *lexer, const Token& self,Expr::Ptr left)
{
  const auto& rule = GetRules(self.type);
  Expr::Ptr right = parse_expression(lexer, rule.lbp - rule.is_right);

  return MAKE_PTR(Expr)(Expr{
    .atom = self,
    .data = BinaryExpr{
      .lhs = MOVE(left),
      .rhs = MOVE(right)
    }
  });
}


Expr::Ptr infix_call(Lexer* lexer, const Token& self, Expr::Ptr left)
{

  std::vector<Expr*> temp_arr;
  // call with args
  if (lexer->peek().type != ETK_RParen)
  {

    for (;;) {

      temp_arr.emplace_back(nullptr) = parse_expression(lexer);

      if (lexer->peek().type == ETK_Comma)
      {
        lexer->consume();
      }
      else break;
    }
  }
  lexer->expect(ETK_RParen);

  Expr* *param_arr = nullptr;
  if(!temp_arr.empty()) {
    param_arr = g_arena.alloc_as<Expr*>(temp_arr.size());

    std::memcpy(param_arr,temp_arr.data(),sizeof(Expr*) * temp_arr.size());
  }

  Expr::Ptr result = MAKE_PTR(Expr)(Expr{
    .atom = self,
    .data = CallExpr{
      .callee = MOVE(left),
      .params = param_arr,
      .params_cnt = static_cast<uint32_t>(temp_arr.size())
    }
  });


  return result;
}

Expr::Ptr infix_dot(Lexer* lexer, const Token& self, Expr::Ptr left)
{
  Expr::Ptr right = MAKE_PTR(Expr)(Expr{
    .atom = lexer->expect(ETK_Identifier),
    .data = IdentExpr {}
  });

  return MAKE_PTR(Expr)(Expr{
    .atom = self,
    .data = MemberExpr {
      .base = MOVE(left),
      .member = MOVE(right)
    }
  });
}


Expr::Ptr infix_arr(Lexer* lexer, const Token& self, Expr::Ptr left)
{
  Expr::Ptr right = parse_expression(lexer);
  lexer->expect(ETK_RBracket);

  return MAKE_PTR(Expr)(Expr{
    .atom = self,
    .data = ArrSubExpr {
      .base = MOVE(left),
      .index = MOVE(right)
    }
  }
  );
}

Expr::Ptr parse_expression(Lexer *lexer, uint8_t rbp /* = 0 */)
{
  // FIX: next() -> peek() + not error=>consume()
  auto left_tok = lexer->peek();

  const PrefixFn prefix_fn = GetRules(left_tok.type).prefix;
  if (!prefix_fn) {
    lexer->error_at(left_tok,"not prefix_fn at this token");
    return nullptr;
  }
  lexer->consume();
  Expr::Ptr left = prefix_fn(lexer,left_tok);

  for (;;) {
    auto op_tok = lexer->peek();
    const auto& rule = GetRules(op_tok.type);
    if (rule.lbp == 0 || rbp >= rule.lbp) {
      break;
    }
    // rule(op).lbp != 0 && rbp < rule(op).lbp

    const InfixFn infix_fn = rule.infix;
    if (!infix_fn) {
      // TODO: what msg is better?
      lexer->error_at(op_tok,"expect binary-op or postfix-op or ';'");
      break;
    }
    lexer->consume();

    left = infix_fn(lexer,op_tok,MOVE(left));
  }

  return left;
}

