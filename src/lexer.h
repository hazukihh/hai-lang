#include "common/Log.h"
#include "common/Defer.hpp"
#include "common/StringView.h"

#include "tokens_def.h"

#include <deque>


// #define CONSUME_CASE_X(Name,Ch)\
//   case Ch :{\
//     tokens.push_back({.type = EToken_Punct,.punct = EPunct_##Name});\
//     break;\
//   }
//
// #define CONSUME_UNARY_OPS_CASE_EXPAND(X_MACRO)\
//   X_MACRO(Add         ,'+')\
//   X_MACRO(Subtract    ,'-')\
//   X_MACRO(Multiply    ,'*')\
//   X_MACRO(Divide      ,'/')\
//   X_MACRO(LParen      ,'(')\
//   X_MACRO(RParen      ,')')\
//   X_MACRO(LBrace      ,'{')\
//   X_MACRO(RBrace      ,'}')\
//   X_MACRO(LBracket    ,'[')\
//   X_MACRO(RBracket    ,']')\
//   X_MACRO(Not         ,'!')\
//   X_MACRO(BitAnd      ,'&')\
//   X_MACRO(BitOr       ,'|')\
//   X_MACRO(BitXor      ,'^')\
//   X_MACRO(BitNot      ,'~')\
//   X_MACRO(Comma       ,',')\
//   X_MACRO(Dot         ,'.')\
//   X_MACRO(Colon       ,':')\
//   X_MACRO(Semicolon   ,';')\
//   X_MACRO(Assign      ,'=')\
//   X_MACRO(LessThan    ,'<')\
//   X_MACRO(GreaterThan ,'>')



struct Lexer
{
  std::string_view input_;

  // TODO: bol,row

  /*
  void consume(char ch)
  {
    Token last;
    if (!tokens.empty()) last = tokens.back();

    switch(ch) {
    case ' ': case '\t': case '\n': {
      // tokens.push_back({EToken_Punct});
      break;
    }
   case '+': {
    tokens.push_back({.type = EToken_Punct, .punct = EPunct_Add});
    break;
    }
    case '-': {
      tokens.push_back({.type = EToken_Punct, .punct = EPunct_Subtract});
      break;
    }
    case '*': {
      tokens.push_back({.type = EToken_Punct, .punct = EPunct_Multiply});
      break;
    }
    case '/': {
      tokens.push_back({.type = EToken_Punct, .punct = EPunct_Divide});
      break;
    }
    case '(': {
      tokens.push_back({.type = EToken_Punct, .punct = EPunct_LParen});
      break;
    }
    case ')': {
      tokens.push_back({.type = EToken_Punct, .punct = EPunct_RParen});
      break;
    }
    case '{': {
      tokens.push_back({.type = EToken_Punct, .punct = EPunct_LBrace});
      break;
    }
    case '}': {
      tokens.push_back({.type = EToken_Punct, .punct = EPunct_RBrace});
      break;
    }
    case '[': {
      tokens.push_back({.type = EToken_Punct, .punct = EPunct_LBracket});
      break;
    }
    case ']': {
      tokens.push_back({.type = EToken_Punct, .punct = EPunct_RBracket});
      break;
    }
    case '!': {
      tokens.push_back({.type = EToken_Punct, .punct = EPunct_Not});
      break;
    }
    case '&': {
      if (last.type != EToken_None && last.type == EToken_Punct && last.punct == EPunct_BitAnd)
      {
        tokens.back().punct = EPunct_And;
      }
      else
      {
        tokens.push_back({.type = EToken_Punct, .punct = EPunct_BitAnd});
      }
      break;
    }
    case '|': {
      if (last.type != EToken_None && last.type == EToken_Punct && last.punct == EPunct_BitOr)
      {
        tokens.back().punct = EPunct_Or;
      }
      else
      {
        tokens.push_back({.type = EToken_Punct, .punct = EPunct_BitOr});
      }
      break;
    }
    case '^': {
      tokens.push_back({.type = EToken_Punct, .punct = EPunct_BitXor});
      break;
    }
    case '~': {
      tokens.push_back({.type = EToken_Punct, .punct = EPunct_BitNot});
      break;
    }
    case ',': {
      tokens.push_back({.type = EToken_Punct, .punct = EPunct_Comma});
      break;
    }
    case '.': {
      tokens.push_back({.type = EToken_Punct, .punct = EPunct_Dot});
      break;
    }
    case ':': {
      tokens.push_back({.type = EToken_Punct, .punct = EPunct_Colon});
      break;
    }
    case ';': {
      tokens.push_back({.type = EToken_Punct, .punct = EPunct_Semicolon});
      break;
    }
    case '=': {
      if (last.type != EToken_None && last.type == EToken_Punct)
      {
        EPunct new_enum;
        switch (last.punct)
        {
        case EPunct_Assign:     new_enum = EPunct_Eq;break;
        case EPunct_Not:     new_enum = EPunct_NotEq;break;
        case EPunct_LessThan:   new_enum = EPunct_LessEq;break;
        case EPunct_GreaterThan:   new_enum = EPunct_GreaterEq;break;
          default: new_enum = last.punct;
        }
        tokens.back().punct = new_enum;
      }
      else
      {
        tokens.push_back({.type = EToken_Punct, .punct = EPunct_Assign});
      }
      break;
    }
    case '<': {
      tokens.push_back({.type = EToken_Punct, .punct = EPunct_LessThan});
      break;
    }
    case '>': {
      tokens.push_back({.type = EToken_Punct, .punct = EPunct_GreaterThan});
      break;
    }
    default: break;
    }
  }
  */


  Token get_token()
  {
    // Trim left-whitespace / Comments
    while(input_.size() > 0)
    {
      input_ = sv_trim_left(input_);

      if(sv_starts_with(input_,SlComment)) {

        // Chop until endline
        size_t pos = input_.find('\n');
        if(pos == StringView::npos) {
          pos = input_.size() - 1;
        }
        input_ = sv_slice(input_,pos+1);

        continue;
      }

      // TODO: MlCommentOpen,MlCommentClose


      break;
    }

    if(input_.empty()) {
      return Token{.type = EToken_EOF};
    }

    // Puncts
    assert(EStr_Puncts[0][1] != '\0' && "should check the binary ops first");
    /**
     * TODO：how to optimize from O(n) to O(1)
     *    ？先hash检查是否是unary ops,如果是就检查下一个字符，是否可以binary
     */
    for(size_t i = 0; i < EPunct_Count; ++i) {
      const char* punct_str = EStr_Puncts[i];
      if(sv_starts_with(input_,punct_str)) {
        size_t punct_str_len = strlen(punct_str);
        input_ = sv_slice(input_,punct_str_len);
        return Token{
          .type = EToken_Punct,
          .punct = static_cast<EPunct>(i)
        };
      }
    }

    // Int literal
    if(isdigit(input_[0])) {
      size_t pos = 0;
      while(pos < input_.size() && isdigit(input_[pos])) {
        ++pos;
      }
      StringView literal_sv = {input_.data(),pos};
      input_ = sv_slice(input_,pos);

      return Token{
        .type = EToken_Literal,
        .literal_index = StrTable_Add(literal_sv)
      };
    }

    // TODO: float literal


    // Identifier / Keyword
    if(IsSymbolStart(input_[0])) {

      size_t pos = 0;
      while(pos < input_.size() && IsSymbol(input_[pos])) {
        ++pos;
      }

      auto sv = StringView{input_.data(),pos};
      input_ = sv_slice(input_,pos);

      // Keyword
      EKeyword keyword_index = IsKeyword(sv);
      if (keyword_index != EKeyword_count)
      {
        return Token{
          .type = EToken_Keyword,
          .keyword = keyword_index
        };
      }


      // Identifier
      return Token{
        .type = EToken_Identifier,
        .identifier_index = StrTable_Add(sv)
      };
    }

    // Unknown character
    LOG_WARN("Unknown character: {}", input_[0]);
    input_ = sv_slice(input_,1);

    return Token{.type = EToken_None};
  }


};
