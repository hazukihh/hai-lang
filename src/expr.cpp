#include "expr.h"

#include <span>

#include "lexer.h"
#include "common/variant_match.h"
#include "common/Assert.h"
#include "symbol_table.h"

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
  MATCH(self.data,
    [&](const CharLitExpr& node) {
      LOG_DEBUG("{:{}}{}or{}","",depth*SPACE_SIZE,node.codepoint,node.val);
    },
    [&](const StrLitExpr& node) {
      fmt::memory_buffer buf;
      if (node.val.empty()) buf.append(sv_from_lit("<empty-str>"));
      else
      {
        buf.append(fmt::format("(1): {}\n(2): ",node.val));
        for (char ch : node.val)
        {
          fmt::format_to(std::back_inserter(buf),"{:d},",ch);
        }
      }
      LOG_DEBUG("{:{}}{}","",depth*SPACE_SIZE,fmt::to_string(buf));

    },
    [&](const auto& _)
    {
      LOG_DEBUG("{:{}}{}","",depth*SPACE_SIZE,get_value(self.atom));
    }
  );


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

/**
 * @brief parse escape char
 *   else like c/c++
 * @param lexer
 * @param tok
 * @param[inout] text
 *   when in, the text is not removed the prefix '\\'.
 *   when out, the text remove the parsed char.
 * @return {char number,ok}
 * TODO: if support \u \U, should return {uint32_t,ok} ??
 */
static std::pair<uint32_t,bool> parse_escape_char(Lexer *lexer,const Token& tok,StringView& text) {
  // removed the prefix '\\'
  text.remove_prefix(1);

  if(text.empty()) [[unlikely]]{
    //  may never can go into here
    lexer->error_at(tok,"CharLiteral lack something after '\\'");
    return {0,false};
  }

  #if 0 // no Octal \nnn, but remain \0
  if('0' <= text[0] && text[0] <= '7') {
    // Octal Number \ddd (1~3 octal digits)
    int val = text[0] - '0';
    int len = 1;
    while(len < 3 && len < text.size() && '0' <= text[len] && text[len] <= '7') {
      val = (val << 3) + (text[len] - '0');
      ++len;
    }
    text.remove_prefix(len);
    // TODO: if val > 255, return ok=false?? or cast(char)(val)
    return {static_cast<char>(val),true};
  }
  #endif

  if (text[0] == 'x') {
    /// Hex Number \xhh, hex fixed 2 hex digits,
    /// because "\xFFFanu" but user may want "\xFF" + "Fanu"

    if(text.size() < 3) {
      lexer->error_at(tok,"\\xhh need fixed 2 hex digits");
      return {0,false};
    }
    if (!isxdigit(text[1]) || !isxdigit(text[2])) {
      lexer->error_at(tok,"the h in \\xhh should be hex digits ");
      return {0,false};
    }

    uint32_t val = int_from_hex(text[1]);
    val = (val << 4) + int_from_hex(text[2]);

    text.remove_prefix(3);

    return {val,true};

  }



  switch(text[0])
  {
  case '0': text.remove_prefix(1);  return {'\0',true};
  case 'a': text.remove_prefix(1);  return {'\a',true};
  case 'b': text.remove_prefix(1);  return {'\b',true};
  case 'f': text.remove_prefix(1);  return {'\f',true};
  case 'n': text.remove_prefix(1);  return {'\n',true};
  case 'r': text.remove_prefix(1);  return {'\r',true};
  case 't': text.remove_prefix(1);  return {'\t',true};
  case 'v': text.remove_prefix(1);  return {'\v',true};
  case '\\': text.remove_prefix(1); return {'\\',true};
  // case '?': text.remove_prefix(1);  return {'\?',true};
  case '\'': text.remove_prefix(1); return {'\'',true};
  case '\"': text.remove_prefix(1); return {'\"',true};
  // [GNU] \e for the ASCII escape character is a GNU C extension.
  case 'e': text.remove_prefix(1); return {27,true};

  // TODO: ERROR / Warning here?. Now,just ok.for-example: look '\z' as 'z'
  default:
    lexer->error_at(tok,fmt::format("illegal escape char '\\{}'",text[0]));
    return {text[0],false};
  }
}


// TODO: Lexer *lexer. lexer->error_at
static std::pair<uint32_t,bool> parse_char_literal(Lexer *lexer,const Token& tok) {
  StringView text = tok.text;

  HAI_ASSERT(text.size()>=2 && text.front() == '\'' && text.back() == '\'');
  text.remove_prefix(1);
  text.remove_suffix(1);


  if(text.empty()) {
    lexer->error_at(tok,"CharLiteral empty");
    return {0,false};
  }

  // Escape Character
  if(text[0] == '\\') {
    if(text.size() < 2) [[unlikely]]{
      //  may never can go into here
      lexer->error_at(tok,"CharLiteral lack something after '\\'");
      return {0,false};
    }

    auto [val,ok] = parse_escape_char(lexer,tok,text);
    if (!ok) {
      return {0,false};
    }
    if (!text.empty()) {
      // char_len > 1
      lexer->error_at(tok,"CharLiteral len > 1");
      return {0,false};
    }
    return {val,true};
  }
  /// text[0] != '\\'

  // TODO: Type-char is "unicode codepoint" or "1 byte" ?? (now) "1 byte"
  if(text.size() > 1) {
    lexer->error_at(tok,"CharLiteral len > 1");
    return {0,false};
  }
  return {text[0],true};

}


// std::pair<size_t,const char*> vs std::pair<size_t,const uint8_t*>
// StringView vs std::span<const uint8_t>
static StringView parse_str_literal(Lexer *lexer,const Token& tok)
{
  StringView text = tok.text;

  HAI_ASSERT(text.size()>=2 && text.front() == '"' && text.back() == '"');
  text.remove_prefix(1);
  text.remove_suffix(1);

  const auto error_case = StringPool::GetInstance().CreateString("");

  if(text.empty()) {
    return error_case;
  }

  fmt::memory_buffer buf;
  while (!text.empty())
  {
    // Escape Character
    if(text[0] == '\\') {
      if(text.size() < 2) [[unlikely]]{
        //  may never can go into here
        lexer->error_at(tok,"StringLiteral lack something after '\\'");
        return error_case;
      }

      auto [val,ok] = parse_escape_char(lexer,tok,text);
      if (!ok) {
        return error_case;
      }
      // TODO: when support \u,should cast to multi-char, and push_back
      buf.push_back(static_cast<char>(val));

    }
    else
    {
      buf.push_back(text[0]);
      text.remove_prefix(1);
    }
  }
  return StringPool::GetInstance().CreateString(fmt::to_string(buf));
}

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
    {ETK_StrLit ,{prefix_primary,nullptr,150}},
    {ETK_Error,{prefix_primary,nullptr,150}},
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
#define MAKE_PTR(Type) ::new (g_arena.alloc<Type>(1)) Type

#endif
/// ============
/// Utils End
/// ============


Expr::Ptr prefix_primary(Lexer *lexer, const Token& self) {
  switch (self.type) {
  case ETK_Error: {
    lexer->isPanic = true;
    return MAKE_PTR(Expr)(Expr{
      .atom = self,
      .data = std::monostate{}
    });
  }break;
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

    auto [val,ok] = parse_char_literal(lexer,self);

    lit.codepoint = val;
    lit.val = val;

    return MAKE_PTR(Expr)(Expr{
      .atom = self,
      .data = lit,
    });
  }break;
  case ETK_StrLit: {
    StrLitExpr str_lit;
    // TODO: Symbol Table / String Pool

    str_lit.val = parse_str_literal(lexer,self);
    // str_lit.val = reinterpret_cast<const uint8_t*>(sv.data());
    // str_lit.count = sv.size();


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
    param_arr = g_arena.alloc<Expr*>(temp_arr.size());

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

  // TODO: left never be nullptr
  if(lexer->isPanic || left == nullptr) {
    return left;
  }

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

