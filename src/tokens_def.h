#ifndef TOKENS_DEF_H
#define TOKENS_DEF_H
#pragma once


#include <cstddef> // size_t
#include <deque>
#include <unordered_set>

#define KEYWORD_ENUM_X(Name) EKeyword_##Name
// #define KEYWORD_ARR_X(Name) [EKeyword_##Name] = #Name
#define KEYWORD_ARR_X(Name) #Name
#define KEYWORD_MAP_X(Name) {#Name,EKeyword_##Name}
#define KEYWORD_EXPAND(X_MACRO)\
  X_MACRO(s8),X_MACRO(u8),\
  X_MACRO(s16),X_MACRO(u16),\
  X_MACRO(s32),X_MACRO(u32),\
  X_MACRO(s64),X_MACRO(u64),\
  X_MACRO(f32),X_MACRO(f64),\
  X_MACRO(string),/*TODO: should builtin the type string ?*/\
  X_MACRO(if),X_MACRO(while),\
  X_MACRO(bool),X_MACRO(void),\
  X_MACRO(false),X_MACRO(true),\
  X_MACRO(break),X_MACRO(continue),\
  X_MACRO(struct),X_MACRO(var),\
  X_MACRO(const), X_MACRO(fn),\
  X_MACRO(return)



enum EKeyword
{
  KEYWORD_EXPAND(KEYWORD_ENUM_X),
  EKeyword_count
};


// TODO: better? use gpref to generate the keyword set(perfect hash)
//    or use trie-tree to match
inline const char* EStr_Keywords[] = {
  KEYWORD_EXPAND(KEYWORD_ARR_X)
};
inline EKeyword IsKeyword(std::string_view sv)
{
  // TODO: flat_map
  static std::unordered_map<std::string_view,EKeyword> keyword_map = {
    KEYWORD_EXPAND(KEYWORD_MAP_X)
  };

  auto iter = keyword_map.find(sv);
  if ( iter != keyword_map.end())
  {
    return iter->second;
  }
  return EKeyword_count;
}


#undef KEYWORD_EXPAND
#undef KEYWORD_ARR_X
#undef KEYWORD_ENUM_X




#define PUNCT_ENUM_X(Name,Str) EPunct_##Name
// #define PUNCT_ARR_X(Name,Str) [EPunct_##Name] = Str
#define PUNCT_ARR_X(Name,Str) Str

// 一元运算符
#define PUNCT_UNARY_OPS_EXPAND(X_MACRO) \
  X_MACRO(Add         ,"+"),\
  X_MACRO(Subtract    ,"-"),\
  X_MACRO(Multiply    ,"*"),\
  X_MACRO(Divide      ,"/"),\
  X_MACRO(LParen      ,"("),\
  X_MACRO(RParen      ,")"),\
  X_MACRO(LBrace      ,"{"),\
  X_MACRO(RBrace      ,"}"),\
  X_MACRO(LBracket    ,"["),\
  X_MACRO(RBracket    ,"]"),\
  X_MACRO(Not         ,"!"),\
  X_MACRO(BitAnd      ,"&"),\
  X_MACRO(BitOr       ,"|"),\
  X_MACRO(BitXor      ,"^"),\
  X_MACRO(BitNot      ,"~"),\
  X_MACRO(Comma       ,","),\
  X_MACRO(Dot         ,"."),\
  X_MACRO(Colon       ,":"),\
  X_MACRO(Semicolon   ,";"),\
  X_MACRO(Assign      ,"="),\
  X_MACRO(LessThan    ,"<"),\
  X_MACRO(GreaterThan ,">"),\
  X_MACRO(SlQuota     ,"'"),\
  X_MACRO(DbQuota     ,"\""),\
  X_MACRO(sharp       ,"#")

  // 二元运算符
#define PUNCT_BINARY_OPS_EXPAND(X_MACRO)\
  X_MACRO(Eq          ,"=="),\
  X_MACRO(NotEq       ,"!="),\
  X_MACRO(LessEq      ,"<="),\
  X_MACRO(GreaterEq   ,">="),\
  X_MACRO(And         ,"&&"),\
  X_MACRO(Or          ,"||")

// TODO: README: must define BINARY_OPS first，because the match now is use the 'for'
#define PUNCT_EXPAND(X_MACRO)\
  PUNCT_BINARY_OPS_EXPAND(X_MACRO),\
  PUNCT_UNARY_OPS_EXPAND(X_MACRO)



/**
 * TODO:
 * deref = multiply = *
 * address-of = BitAnd  = &
 */
enum EPunct
{
  PUNCT_EXPAND(PUNCT_ENUM_X),
  EPunct_Count
};


inline const char* EStr_Puncts[] = {
  PUNCT_EXPAND(PUNCT_ARR_X)
};


#undef PUNCT_EXPAND
#undef PUNCT_UNARY_OPS_EXPAND
#undef PUNCT_BINARY_OPS_EXPAND
#undef PUNCT_ARR_X
#undef PUNCT_ENUM_X


// signle Comment str
constexpr const char* SlComment = "//";


#define TOKEN_TYPE_ENUM_X(Name)  EToken_##Name
// #define TOKEN_TYPE_ARR_X(Name) [EToken_##Name] = #Name
#define TOKEN_TYPE_ARR_X(Name) #Name
#define TOKEN_TYPE_EXPAND(XX)\
  XX(None),\
  XX(Keyword),\
  XX(Punct),/*punctuation*/\
  XX(Identifier), /*标识符*/\
  XX(Literal),/*字面量*/\
  XX(Comment),/*注释*/\
  XX(Whitespace),\
  XX(EOF)


enum ETokenType
{
  TOKEN_TYPE_EXPAND(TOKEN_TYPE_ENUM_X)
};
inline const char* EStr_TokenType[] = {
  TOKEN_TYPE_EXPAND(TOKEN_TYPE_ARR_X)
};


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

// Symbol = Identifier + Keyword

inline bool IsSymbolStart(char ch)
{
  return isalpha(ch) || ch == '_';
}

inline bool IsSymbol(char ch)
{
  return isalnum(ch) || ch == '_';
}


struct Token
{
  ETokenType type = EToken_None;
  union
  {
    EKeyword keyword = static_cast<EKeyword>(0);
    EPunct punct;
    // TODO: store the index of the string in the string table
    size_t identifier_index;
    // TODO: store the value, use a union for different types of literals ?
    size_t literal_index;
  };

  [[nodiscard]] const char* to_str() const
  {
    switch (type)
    {
    case EToken_Keyword: return EStr_Keywords[keyword];
    case EToken_Punct: return EStr_Puncts[punct];
    case EToken_Identifier: return StrTable_Get(identifier_index).data();
    case EToken_Literal: return StrTable_Get(literal_index).data();
    case EToken_Comment: return "<Comment>";
    case EToken_Whitespace: return "<WhiterSpace>";
    case EToken_None: return "<Unknown>";
    case EToken_EOF: return "<EOF>";
    }
  }
};



#endif // TOKENS_DEF_H
