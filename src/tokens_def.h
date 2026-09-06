#ifndef TOKENS_DEF_H
#define TOKENS_DEF_H
#pragma once

#include <parallel_hashmap/phmap.h>


#include "common/Assert.h"
#include "common/StringView.h"


// Single Comment str and Multi Comment Left+Right str
constexpr StringView SlComment = "//";
constexpr StringView LeftMlComment = "/*";
constexpr StringView RightMlComment = "*/";
constexpr char MlCommentCharSet_AND_LF[] = "*/\n";
static_assert(MlCommentCharSet_AND_LF[std::size(MlCommentCharSet_AND_LF)-2] == '\n');

// XX(Name)
#define KEYWORD_EXPAND(XX)\
  XX(s8),XX(u8),\
  XX(s16),XX(u16),\
  XX(s32),XX(u32),\
  XX(s64),XX(u64),\
  XX(f32),XX(f64),\
  XX(string),/*TODO: should builtin the type string ?*/\
  XX(bool),XX(void),\
  XX(false),XX(true),\
  XX(struct),XX(var),\
  XX(const), XX(fn),\
  XX(if),XX(else),\
  XX(while),XX(for),\
  XX(break),XX(continue),\
  XX(return)

#define KEYWORD_ARR_X(Name) #Name
inline const char* kKeywords_CStr[] = {
  KEYWORD_EXPAND(KEYWORD_ARR_X)
};
#undef KEYWORD_ARR_X


// one-char运算符
#define PUNCT_1CHAR_OPS_EXPAND(XX) \
  XX(Plus        ,"+"),\
  XX(Minus       ,"-"),\
  XX(Star        ,"*"),\
  XX(Slash       ,"/"),\
  XX(Mod         ,"%"),\
  XX(LParen      ,"("),\
  XX(RParen      ,")"),\
  XX(Assign      ,"="),\
  XX(LBrace      ,"{"),\
  XX(RBrace      ,"}"),\
  XX(LBracket    ,"["),\
  XX(RBracket    ,"]"),\
  XX(Not         ,"!"),\
  XX(BitAnd      ,"&"),\
  XX(BitOr       ,"|"),\
  XX(BitXor      ,"^"),\
  XX(BitNot      ,"~"),\
  XX(Comma       ,","),\
  XX(Dot         ,"."),\
  XX(Colon       ,":"),\
  XX(Semi        ,";"),\
  XX(LessThan    ,"<"),\
  XX(GreaterThan ,">"),\
  XX(Sharp       ,"#"),\
  XX(Question    ,"?"),\
  XX(Dollar      ,"$")

// TODO: maybe operators don't need '\'' and '\"'
// XX(SlQuota     ,"'"),\
// XX(DbQuota     ,"\"")

// two-char运算符
#define PUNCT_2CHAR_OPS_EXPAND(XX)\
  XX(Dot3        ,"..."),\
  XX(Eq          ,"=="),\
  XX(NotEq       ,"!="),\
  XX(LessEq      ,"<="),\
  XX(GreaterEq   ,">="),\
  XX(And         ,"&&"),\
  XX(Or          ,"||"),\
  XX(LShift      ,"<<"),\
  XX(RShift      ,">>"),\
  XX(RArrow      ,"->")

// Tip: README: must define 2CHAR_OPS or 3Char first，because the lexer now is  using the 'for' to match puncts

// XX(Name,Str)
#define PUNCT_EXPAND(X_MACRO)\
  PUNCT_2CHAR_OPS_EXPAND(X_MACRO),\
  PUNCT_1CHAR_OPS_EXPAND(X_MACRO)


#define PUNCT_ARR_X(Name,Str) Str
inline const char* kPuncts_CStr[] = {
  PUNCT_EXPAND(PUNCT_ARR_X)
};
#undef PUNCT_ARR_X


#define KEYWORD_ENUM_X(Name) ETK_##Name
#define PUNCT_ENUM_X(Name,Str) ETK_##Name
// TODO: which solution better ?: 使用 ascii(0-127)值 表示 单字符TOKEN,其他类型TOKEN从128开始??
enum ETokenType
{
  ETK_Error,
  ETK_EOF,
  ETK_Whitespace,
  ETK_Comment,
  ETK_Identifier,
  ETK_IntLit,
  ETK_FlLit,
  ETK_CharLit,
  ETK_StrLit,

  ETK_KEYWORD_START,
  _etk_keyword_start = ETK_KEYWORD_START - 1,
  KEYWORD_EXPAND(KEYWORD_ENUM_X),
  ETK_KEYWORD_END,

  ETK_PUNCT_START = ETK_KEYWORD_END,
  _etk_punct_start = ETK_PUNCT_START - 1,
  PUNCT_EXPAND(PUNCT_ENUM_X),
  ETK_PUNCT_END,
  ETK_COUNT = ETK_PUNCT_END,
  ETK_None = ETK_COUNT
};
static_assert(ETK_KEYWORD_END - ETK_KEYWORD_START == std::size(kKeywords_CStr));
static_assert(ETK_PUNCT_END - ETK_PUNCT_START == std::size(kPuncts_CStr));

#undef KEYWORD_ENUM_X
#undef PUNCT_ENUM_X



#define STRINGFY(x) #x
#define ETOKEN_KEYWORD_NAME_X(Name) STRINGFY(ETK_##Name)
#define ETOKEN_PUNCT_NAME_X(Name,Str) STRINGFY(ETK_##Name)
[[nodiscard]]
inline StringView ETokenType_to_Str(ETokenType type)
{
  static const char* kETokenTypeName[] = {
    /*[ETK_None] =*/ "ETK_Error",
    /*[ETK_EOF] =*/ "ETK_EOF",
    /*[ETK_Whitespace] =*/ "ETK_Whitespace",
    /*[ETK_Comment] =*/ "ETK_Comment",
    /*[ETK_Identifier] =*/ "ETK_Identifier",
    /*[ETK_IntLit] =*/ "ETK_IntLit",
    /*[ETK_FlLit] =*/ "ETK_FlLit",
    /*[ETK_CharLit]*/ "ETK_CharLit",
    /*[ETK_StrLit] =*/ "ETK_StrLit",
    KEYWORD_EXPAND(ETOKEN_KEYWORD_NAME_X),
    PUNCT_EXPAND(ETOKEN_PUNCT_NAME_X),
    "ETK_None"
  };
  static_assert(ETK_COUNT+1 == std::size(kETokenTypeName));

  return kETokenTypeName[type];
}
#undef STRINGFY
#undef ETOKEN_KEYWORD_NAME_X
#undef ETOKEN_PUNCT_NAME_X

// inline ETokenType Tag_Str_to_Enum(std::string_view sv);

#define KEYWORD_MAP_X(Name) {#Name,ETK_##Name}
inline ETokenType KeywordStr_to_ETokenType(std::string_view sv)
{
  static phmap::flat_hash_map<std::string_view,ETokenType> keyword_map = {
    KEYWORD_EXPAND(KEYWORD_MAP_X)
  };

  const auto iter = keyword_map.find(sv);
  if ( iter != keyword_map.end())
  {
    return iter->second;
  }
  return ETK_Error;
}
#undef KEYWORD_MAP_X


#undef PUNCT_1CHAR_OPS_EXPAND
#undef PUNCT_2CHAR_OPS_EXPAND
#undef PUNCT_EXPAND
#undef KEYWORD_EXPAND

inline const char* GetKeywordCStr(ETokenType type) {
  if (ETK_KEYWORD_START <= type && type < ETK_KEYWORD_END)
  {
    return kKeywords_CStr[type - ETK_KEYWORD_START];
  }
  HAI_ASSERT(false && "type is not keyword type in ETokenType");
  return "";

}

inline const char* GetPunctCStr(ETokenType type) {
  if (ETK_PUNCT_START <= type && type < ETK_PUNCT_END)
  {
    return kPuncts_CStr[type - ETK_PUNCT_START];
  }
  HAI_ASSERT(false && "type is not punct type in ETokenType");
  return "";
}



// Symbol = Identifier + Keyword

inline bool IsSymbolStart(char ch)
{
  return is_alpha(ch) || ch == '_';
}

inline bool IsSymbol(char ch)
{
  return is_alnum(ch) || ch == '_';
}

struct TokenLoc
{
  // std::string filename; // TODO: filename,FILE_ID
  uint32_t line = 0;
  const char* line_start = nullptr;

  uint32_t column(const char* text_start) const
  {
    HAI_ASSERT(line_start!=nullptr && text_start!=nullptr
      && line_start <=  text_start);
    return static_cast<uint32_t>(text_start - line_start);
  }
};

/**
 * @note the value of identifier,literals  is stored by the string_view of input_src
 */
// TODO: Optimize the size of struct Token, more smaller. Clang: uint32: FILE_ID + Loca
struct Token
{
  ETokenType type = ETK_None;

  StringView text;

  TokenLoc loc;

  // Identifier,IntLit,FlLit,StrLit will return .text, others return type_tag_str
  [[nodiscard]] StringView to_str() const
  {
    switch (type)
    {
      case ETK_Error: return "<Error>";
      case ETK_EOF: return "<EOF>";
      case ETK_Whitespace: return "<WhiterSpace>";
      case ETK_None:       // fallthrough
      case ETK_Comment:    // fallthrough
      case ETK_Identifier: // fallthrough
      case ETK_IntLit:     // fallthrough
      case ETK_FlLit:      // fallthrough
      case ETK_CharLit:    // fallthrough
      case ETK_StrLit:     // fallthrough
        return text;
      default: break;
    }
    if (ETK_KEYWORD_START <= type && type < ETK_KEYWORD_END)
    {
      return GetKeywordCStr(type);
    }
    if (ETK_PUNCT_START <= type && type < ETK_PUNCT_END)
    {
      return GetPunctCStr(type);
    }
    HAI_UNREACHABLE();
    return "";
  }
};






#endif // TOKENS_DEF_H
