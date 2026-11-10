#include "expr.h"

#include <range/v3/view/filter.hpp>
#include <range/v3/range/conversion.hpp>

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
    [&](const IntLitExpr& node) {
      LOG_DEBUG("{:{}}{} = {}, suffix={}","",depth*SPACE_SIZE,
        get_value(self.atom),node.val,node.suffix);
    },
    [&](const FloatLitExpr& node)
    {
      LOG_DEBUG("{:{}}{} = {}, suffix={}","",depth*SPACE_SIZE,
        get_value(self.atom),node.val,node.suffix);
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
  // 0 lbp: EOF ; )
  // ( rbp: 0
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
  default:
    lexer->error_at(tok,fmt::format("illegal escape char '\\{}'",text[0]));
    return {text[0],false};
  }
}



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

  // TODO: Type-char is "unicode codepoint" as int/u32 or "1 byte" as u8?? (now) "1 byte"
  if(text.size() > 1) {
    lexer->error_at(tok,"CharLiteral len > 1");
    return {0,false};
  }
  return {text[0],true};

}

// TODO: now use String Pool(Arena for string): StringPool::GetInstance().CreateString(sv);
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




struct NumericLiteralParser {

  enum class EIntBase : uint8_t {
    _10 = 10,
    _2 = 2,
    _8 = 8,
    _16 = 16
  };

  union { // NumericLiteral
    IntLitExpr int_lit = {};
    FloatLitExpr fl_lit;
  } result;

  EIntBase base = EIntBase::_10;
  StringView val_text;
  // 0: integer/float, 1: exponent, 2: suffix
  //TIPS: 1: empty? 2:empty?
  StringView texts[3];
  bool is_float = false;
  std::string error_msg;


  static bool is_digit_base(uint8_t c,const EIntBase _base = EIntBase::_10) {
    switch(static_cast<uint8_t>(_base)) {
    case 2: return c == '0' || c == '1';
    case 8: return '0' <= c && c <= '7';
    case 10: return is_digit(c);
    case 16: return isxdigit(c);
    default: return false;
    }
  }
/**
  Prefix ::= 0b | 0B | 0o | 0O | 0x | 0X
  Digit ::= B_Digit | O_Digit | X_Digit | D_Digit
  Album ::= [0-9a-zA-Z]

  Hex_Literal ::= Prefix Digit [Digit_]* (.[Digit_]*)? ([pP] [+-]? [0-9] [0-9_]*)? (Album [Album_]*)?
  Dec_Literal ::= Digit [Digit_]* (.[Digit_]*)? ([eE] [+-]? [0-9] [0-9_]*)? (Album [Album_]*)?
  Other_Literal ::= Prefix Digit [Digit_]* (Album [Album_]*)?

 */
  bool parse_pp_number(const Token& tok)
  {
    const char *p = tok.text.data();
    const char *end = tok.text.data() + tok.text.size();

    // Prefix
    if (p[0] == '0' && p+1 < end)
    {
      char c1 = p[1];
      switch(c1) {
      case 'x':
      case 'X':
        if(p+2 >= end) {
          error_msg = "0x prefix should be followed by hex digit";
          return false;
        }
        base = EIntBase::_16;
        p += 2;
        break;
      case 'o':
      case 'O':
        if(p+2 >= end) {
          error_msg = "0o prefix should be followed by octal digit";
          return false;
        }
        base = EIntBase::_8;
        p += 2;
        break;
      case 'b':
      case 'B':
        if(p+2 >= end) {
          error_msg = "0b prefix should be followed by binary digit";
          return false;
        }
        base = EIntBase::_2;
        p += 2;
        break;
      default:
        break;
      }
    }

    const char *digits_begin = p;
    // Digit
    if(p >= end || !is_digit_base(*p,base)) {
      if(base == EIntBase::_16)
        error_msg = "invalid literal,0x prefix must be followed by hex digit";
      else if(base == EIntBase::_8)
        error_msg = "invalid literal,0o prefix must be followed by oct digit";
      else if(base == EIntBase::_2)
        error_msg = "invalid literal,0b prefix must be followed by bin digit";
      else
        error_msg = "invalid literal,has invalid dec digit";

      return false;
    }
    p += 1;

    // [Digit_]*
    while(p < end && (is_digit_base(*p,base) || *p == '_'))
    {
      p += 1;
    }

    // (.[Digit__]*)? && (is_hex || is_dec)
    if(p < end && *p == '.')
    {
      if(!(base == EIntBase::_10 || base == EIntBase::_16)) {
        error_msg = "invalid char '.' in literal";
        return false;
      }
      is_float = true;
      p += 1;
      // [Digit__]*
      while(p < end && (is_digit_base(*p,base) || *p == '_'))
      {
        p += 1;
      }
    }

    texts[0] = StringView{digits_begin,p};



    // is_hex => ([pP] [+-]? [0-9] [0-9_]*)?
    // is_dec => ([eE] [+-]? [0-9] [0-9_]*)?
    if(p < end && (base == EIntBase::_16 && (*p == 'p' || *p == 'P')
      || base == EIntBase::_10 && (*p == 'e' || *p == 'E'))
    ){
      // [pP] | [eE]
      is_float = true;
      p += 1;

      const char *exp_begin = p;

      // [+-]?
      if(p < end && (*p == '+' || *p == '-')) {
        p += 1;
      }

      // [0-9]
      if(p >= end || !is_digit(*p)) {
        error_msg = "expected dec digits after exponent";
        return false;
      }
      p += 1;
      // [0-9_]*
      while(p < end && (is_digit(*p) || *p == '_')) {
        p +=1;
      }

      texts[1] = StringView{exp_begin,p};
    }

    if (digits_begin == p)
    {
      error_msg = "invalid number literal";
      return false;
    }

    val_text = StringView{digits_begin,p};

    // suffix: ([alnum][alnum_]*)?
    const char *suffix_begin = p;
    if(p < end && is_alnum(*p)) {

      p += 1;
      while(p < end && (is_alnum(*p) || *p == '_')) {
        p += 1;
      }
      texts[2] = StringView{suffix_begin,p};
    }
    if(p < end) {
      error_msg = fmt::format("number literal suffix \"{}\" has invalid char '{}'",StringView{suffix_begin,end},*p);
      return false;
    }

    return true;
  }


  bool parse(const Token& tok)
  {
    if(!parse_pp_number(tok)) {
      return false;
    }

    // TODO: use the-arena-for-std::string to store the temp string 'val_text_remove_underscore' or not??
    #if 0
    std::string val_text_remove_underscore;
    val_text_remove_underscore.reserve(val_text.size());
    for(char c : val_text) {
      if(c != '_') {
        val_text_remove_underscore.push_back(c);
      }
    }
    #else
    auto val_text_remove_underscore = val_text
      | ranges::views::filter([](char c){return c != '_';})
      | ranges::to<std::string>();
    #endif

    std::from_chars_result res;
    if(is_float) {
      res = ::std::from_chars(
        val_text_remove_underscore.data(),
        val_text_remove_underscore.data() + val_text_remove_underscore.size(),
        result.fl_lit.val,
        base == EIntBase::_16 ? std::chars_format::hex : std::chars_format::general
      );
      result.fl_lit.suffix = texts[2];
    }
    else {
      res = ::std::from_chars(
        val_text_remove_underscore.data(),
        val_text_remove_underscore.data() + val_text_remove_underscore.size(),
        result.int_lit.val,
        static_cast<int>(base)
      );
      result.int_lit.suffix = texts[2];
    }

    if(res.ec != std::errc{}) {
      error_msg = std::error_condition(res.ec).message();
      return false;
    }
    if(res.ptr != val_text_remove_underscore.data() + val_text_remove_underscore.size()) {
      error_msg = "Only parsed part of NumberLit";
      return false;
    }

    return true;
  }


};

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
    {NumberLit ,{prefix_primary,nullptr,150}},
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
#if 0 // the old codes for parse number literal
  case ETK_IntLit: {
    IntLitExpr int_lit;

    auto [ptr,ec] =std::from_chars(
      self.text.data() + (self.int_base == 10 ? 0 : 2),
        self.text.data()+self.text.size(),
        int_lit.val,
        self.int_base);

    if (ec != std::errc{})
    {

      lexer->error_at(self,std::error_condition(ec).message());
      return MAKE_PTR(Expr)(Expr{
        .atom = self,
        .data = std::monostate{}
      });
    }
    if (ptr != self.text.data()+self.text.size())
    {

      lexer->error_at(self,"Only parsed part of IntLiteral");
      return MAKE_PTR(Expr)(Expr{
        .atom = self,
        .data = std::monostate{}
      });
    }

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
#else
  case NumberLit:{
    NumericLiteralParser parser;
    if(!parser.parse(self)) {
      lexer->error_at(self,parser.error_msg);
      return MAKE_PTR(Expr)(Expr{
        .atom = self,
        .data = std::monostate{}
      });
    }
    if(parser.is_float) {
      return MAKE_PTR(Expr)(Expr{
        .atom = self,
        .data = parser.result.fl_lit
      });
    } else {
      return MAKE_PTR(Expr)(Expr{
        .atom = self,
        .data = parser.result.int_lit
      });
    }

  }break;
#endif
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

  // TODO: left never be nullptr?? when parse_expression return nullptr,left = prefix_fn(...) maybe nullptr
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
      // TODO: what msg is better, when infix_fn is nullptr?
      lexer->error_at(op_tok,"expect binary-op or postfix-op or ';'");
      break;
    }
    lexer->consume();

    left = infix_fn(lexer,op_tok,MOVE(left));
  }

  return left;
}

