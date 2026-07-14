#ifndef TOKENS_DEF_H
#define TOKENS_DEF_H
#pragma once

#include <cstddef> // size_t
#include <deque>
#include <unordered_set>

#include "version.h"


// Single Comment str
constexpr const char* SlComment = "//";

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
  XX(if),XX(while),\
  XX(break),XX(continue),\
  XX(return)



// one-char运算符
#define PUNCT_1CHAR_OPS_EXPAND(XX) \
  XX(Plus        ,"+"),\
  XX(Minus       ,"-"),\
  XX(Star        ,"*"),\
  XX(Slash       ,"/"),\
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
  XX(SlQuota     ,"'"),\
  XX(DbQuota     ,"\""),\
  XX(sharp       ,"#")

// two-char运算符
#define PUNCT_2CHAR_OPS_EXPAND(XX)\
  XX(Eq          ,"=="),\
  XX(NotEq       ,"!="),\
  XX(LessEq      ,"<="),\
  XX(GreaterEq   ,">="),\
  XX(And         ,"&&"),\
  XX(Or          ,"||"),\
  XX(LShift      ,"<<"),\
  XX(RShift      ,">>")

// TODO: README: must define 2CHAR_OPS first，because the match now is use the 'for'
// XX(Name,Str)
#define PUNCT_EXPAND(X_MACRO)\
  PUNCT_2CHAR_OPS_EXPAND(X_MACRO),\
  PUNCT_1CHAR_OPS_EXPAND(X_MACRO)





// TODO: merge EKeyword,EPunct into ETokenType and replace the EToken_Keyword,EToken_Punct
#define KEYWORD_ENUM_X(Name) ETK_##Name
#define PUNCT_ENUM_X(Name,Str) ETK_##Name

enum ETokenType
{
  ETK_None,
  ETK_EOF,
  ETK_Whitespace,
  ETK_Comment,
  ETK_Identifier,
  ETK_IntLit,
  ETK_FlLit,
  ETK_StrLit,

  ETK_KEYWORD_START,
  __etk_keyword_start = ETK_KEYWORD_START - 1,
  KEYWORD_EXPAND(KEYWORD_ENUM_X),
  ETK_KEYWORD_END,

  ETK_PUNCT_START = ETK_KEYWORD_END,
  __etk_punct_start = ETK_PUNCT_START - 1,
  PUNCT_EXPAND(PUNCT_ENUM_X),
  ETK_PUNCT_END
};

#undef KEYWORD_ENUM_X
#undef PUNCT_ENUM_X

#define STRINGFY(x) #x
#define ETOKEN_KEYWORD_NAME(Name) STRINGFY(ETK_##Name)
#define ETOKEN_PUNCT_NAME(Name,Str) STRINGFY(ETK_##Name)

inline const char* kETokenTypeName[] = {
  /*[ETK_None] =*/ "ETK_None",
  /*[ETK_EOF] =*/ "ETK_EOF",
  /*[ETK_Whitespace] =*/ "ETK_Whitespace",
  /*[ETK_Comment] =*/ "ETK_Comment",
  /*[ETK_Identifier] =*/ "ETK_Identifier",
  /*[ETK_IntLit] =*/ "ETK_IntLit",
  /*[ETK_FlLit] =*/ "ETK_FlLit",
  /*[ETK_StrLit] =*/ "ETK_StrLit",
  KEYWORD_EXPAND(ETOKEN_KEYWORD_NAME),
  PUNCT_EXPAND(ETOKEN_PUNCT_NAME)
};

#undef STRINGFY
#undef ETOKEN_KEYWORD_NAME
#undef ETOKEN_PUNCT_NAME


#define KEYWORD_ARR_X(Name) #Name
// TODO: better? use gpref to generate the keyword set(perfect hash)
//    or use trie-tree to match
inline const char* kEStr_Keywords[] = {
  KEYWORD_EXPAND(KEYWORD_ARR_X)
};
#undef KEYWORD_ARR_X

#define PUNCT_ARR_X(Name,Str) Str
inline const char* kEStr_Puncts[] = {
  PUNCT_EXPAND(PUNCT_ARR_X)
};
#undef PUNCT_ARR_X

inline const char* EStrKeyword(ETokenType type) {
  assert(ETK_KEYWORD_START <= type && type < ETK_KEYWORD_END && "not keyword type in ETokenType");

  return kEStr_Keywords[type - ETK_KEYWORD_START];
}

inline const char* EStrPunct(ETokenType type) {
  assert(ETK_PUNCT_START <= type && type < ETK_PUNCT_END && "not punct type in ETokenType");

  return kEStr_Puncts[type - ETK_PUNCT_START];
}



#define KEYWORD_MAP_X(Name) {#Name,ETK_##Name}
inline ETokenType KeywordId(std::string_view sv)
{
  // TODO: flat_map
  static std::unordered_map<std::string_view,ETokenType> keyword_map = {
    KEYWORD_EXPAND(KEYWORD_MAP_X)
  };

  const auto iter = keyword_map.find(sv);
  if ( iter != keyword_map.end())
  {
    return iter->second;
  }
  return ETK_None;
}
#undef KEYWORD_MAP_X




#undef PUNCT_1CHAR_OPS_EXPAND
#undef PUNCT_2CHAR_OPS_EXPAND
#undef PUNCT_EXPAND
#undef KEYWORD_EXPAND



// Symbol = Identifier + Keyword

inline bool IsSymbolStart(char ch)
{
  return isalpha(ch) || ch == '_';
}

inline bool IsSymbol(char ch)
{
  return isalnum(ch) || ch == '_';
}

/**
 * @note the value of identifier,literals  is stored by the string_view of input_src
 */

struct Token
{
  ETokenType type = ETK_None;

  StringView text;

  uint32_t line = 0;
  uint32_t column = 0;

  [[nodiscard]] StringView to_str() const
  {
    switch (type)
    {
      case ETK_None: return "<Unknown>";
      case ETK_EOF: return "<EOF>";
      case ETK_Whitespace: return "<WhiterSpace>";
      case ETK_Comment: return "<Comment>";
      case ETK_Identifier:
      case ETK_IntLit:
      case ETK_FlLit:
      case ETK_StrLit:
        return text;
      default: break;
    }
    if (ETK_KEYWORD_START <= type && type < ETK_KEYWORD_END)
    {
      return EStrKeyword(type);
    }
    if (ETK_PUNCT_START <= type && type < ETK_PUNCT_END)
    {
      return EStrPunct(type);
    }
    return "<Unknown>";
  }
};



/*
inline std::deque<std::string> StrTable;
// TODO: flat hash map
inline std::unordered_map<std::string_view,size_t> StrTableMap;
inline size_t StrTable_Add(std::string_view sv)
{
  auto iter = StrTableMap.find(sv);
  if (iter == StrTableMap.end())
  {
    StrTable.emplace_back(sv);
    StrTableMap[StrTable.back()] = StrTable.size() - 1;
    return StrTable.size() - 1;
  }

  return iter->second;
}
inline std::string_view StrTable_Get(const size_t index)
{
  assert(index < StrTable.size() && "out of range of StrTable");
  return StrTable[index];
}
*/


#endif // TOKENS_DEF_H
