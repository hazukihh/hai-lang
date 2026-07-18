#pragma once
#include "common/Log.h"
#include "common/Defer.hpp"
#include "common/StringView.h"

#include "tokens_def.h"

#include <deque>


inline void error_at(const Token& t,StringView msg) {
  fmt::memory_buffer buf;
  fmt::format_to(std::back_inserter(buf),
    "at line {}({})\n{}\n",
      t.line,t.column,
      StringView{t.text.data() - t.column,t.column + t.text.size()}
    );
  size_t offset = t.column;
  for (int i =0;i < offset;++i)
  {
    buf.push_back(' ');
  }
  fmt::format_to(std::back_inserter(buf),
    "^ got {}, {}",get_type_tag_str(t.type),msg);

  LOG_ERROR(fmt::to_string(buf));
  exit(1);
}

inline void error_at(const Token& t,ETokenType expected) {
  error_at(t,fmt::format("expected \"{}\"",get_type_tag_str(expected)));
}

struct Lexer
{
  std::string_view input_;

  // base of row
  const char* base_;
  uint32_t row_ = 1;


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

  [[nodiscard]]
  Token get_token()
  {
    // Trim left-whitespace / Comments
    while(!input_.empty())
    {
      // input_ = sv_trim_left(input_);
      {
        size_t i = 0;
        size_t size = input_.size();
        const char* data = input_.data();
        while (i < size && isspace(data[i])) {
          if (data[i] == '\n') {
            row_ += 1;
            base_ = data + i + 1;
          }

          i += 1;
        }
        input_ =  StringView{data + i, size - i};
      }

      if(sv_starts_with(input_,SlComment)) {

        // Chop until endline
        size_t pos = input_.find('\n');
        if(pos == StringView::npos) {
          pos = input_.size();
        } else {
          row_ += 1;
          base_ = input_.data() + pos + 1;
        }
        input_ = sv_slice(input_,pos+1);

        continue;
      }

      // TODO: MlCommentOpen,MlCommentClose


      break;
    }

    if(input_.empty()) {
      return Token{
        .type = ETK_EOF,
        .text = input_,
        .line = row_,
        .column = static_cast<uint32_t>(input_.data() - base_)
      };
    }

    // Puncts
    assert(kEStr_Puncts[0][1] != '\0' && "should check the 2-char ops first");
    /**
     * TODO：how to optimize from O(n) to O(1)
     *    ？先hash检查是否是one char ops,如果是就检查下一个字符，是否可以2 char ops
     */
    for(size_t i = ETK_PUNCT_START; i < ETK_PUNCT_END; ++i) {
      const char* punct_str = EStrPunct(static_cast<ETokenType>(i));

      if(sv_starts_with(input_,punct_str)) {
        auto punct_str_len = strlen(punct_str);

        StringView sv = sv_slice(input_,0,punct_str_len);
        input_ = sv_slice(input_,punct_str_len);

        return Token{
          .type = static_cast<ETokenType>(i),
          .text = sv,
          .line = row_,
          .column = static_cast<uint32_t>(sv.data() - base_)
        };
      }
    }

    // Int literal
    // TODO: hex: 0X/0x...; Oct 0O/0o...
    // TODO: float literal
    if(isdigit(input_[0])) {
      size_t pos = 0;
      while(pos < input_.size() && isdigit(input_[pos])) {
        ++pos;
      }
      StringView literal_sv = {input_.data(),pos};
      input_ = sv_slice(input_,pos);

      return Token{
        .type = ETK_IntLit,
        .text = literal_sv,
        .line = row_,
        .column = static_cast<uint32_t>(literal_sv.data() - base_)
      };
    }

    // TODO: how to deal with Conflict in Punct '\"' '\'' => remove them from puncts
    // String literal "..."
    if (input_[0] == '"')
    {
      size_t pos = 1;
      while (pos < input_.size() && input_[pos] != '"' && input_[pos] != '\n')
      {
        ++pos;
      }

      // pos.* == '\"'        => input_[0:pos+1)
      // TODO: Tip: warning: lack Right-'\"', but assume there has one and correct. Or directly error here
      // pos == input_.size() => input_[0:pos+1) => input_[0:size()+1) => input_[0:size()) => input_[0:pos)
      // pos.* == '\n'        => input_[0:pos) , leave the rest for next-get in order to support row_/base_
      StringView sv;
      if (pos < input_.size() && input_[pos] == '"') {
        sv = StringView{input_.data(),pos+1};
        input_ = sv_slice(input_,pos+1);
      } else {
        LOG_WARN("StringLiteral lack Right-'\"'");
        sv = StringView{input_.data(),pos};
        input_ = sv_slice(input_,pos);

        // TODO: Option for Strict Mode
        error_at(Token{
          .type = ETK_StrLit,
          .text = sv,
          .line = row_,
          .column = static_cast<uint32_t>(sv.data() - base_)
        },"StringLiteral lack Right-'\"'(Strict Mode)");
      }
      return Token{
        .type = ETK_StrLit,
        .text = sv,
        .line = row_,
        .column = static_cast<uint32_t>(sv.data() - base_)
      };
    }

    // Char literal '...'
    if (input_[0] == '\'')
    {
      size_t pos = 1;
      while (pos < input_.size() && input_[pos] != '\'' && input_[pos] != '\n')
      {
        ++pos;
      }

      // TODO: Similar to StringLit. Should be error when the char len > 1 ? or delay ?
      StringView sv;
      if (pos < input_.size() && input_[pos] == '\'') {
        sv = StringView{input_.data(),pos+1};
        input_ = sv_slice(input_,pos+1);
      } else {
        LOG_WARN("CharLiteral lack Right-'\''");
        sv = StringView{input_.data(),pos};
        input_ = sv_slice(input_,pos);

        // TODO: Option for Strict Mode
        error_at(Token{
          .type = ETK_CharLit,
          .text = sv,
          .line = row_,
          .column = static_cast<uint32_t>(sv.data() - base_)
        },"CharLiteral lack Right-'\''(Strict Mode)");
      }
      return Token{
        .type = ETK_CharLit,
        .text = sv,
        .line = row_,
        .column = static_cast<uint32_t>(sv.data() - base_)
      };
    }


    // Identifier / Keyword
    if(IsSymbolStart(input_[0])) {

      size_t pos = 0;
      while(pos < input_.size() && IsSymbol(input_[pos])) {
        ++pos;
      }

      auto sv = StringView{input_.data(),pos};
      input_ = sv_slice(input_,pos);

      // Keyword
      ETokenType keyword_index = KeywordId(sv);
      if (keyword_index != ETK_None)
      {
        return Token{
          .type = keyword_index,
          .text = sv,
          .line = row_,
          .column = static_cast<uint32_t>(sv.data() - base_)
        };
      }


      // Identifier
      return Token{
        .type = ETK_Identifier,
        .text = sv,
        .line = row_,
        .column = static_cast<uint32_t>(sv.data() - base_)
      };
    }

    // Unknown character
    LOG_WARN("Unknown character: {}", input_[0]);
    Token unknown = {
      .type = ETK_None,
      .text = sv_slice(input_,0,1),
      .line = row_,
      .column = static_cast<uint32_t>(input_.data() - base_)
    };
    input_ = sv_slice(input_,1);
    return unknown;
  }

  [[nodiscard]]
  Token peek() {
    Lexer lexer_copy = *this;
    return lexer_copy.get_token();
  }

  [[nodiscard]]
  Token next() {
    return get_token();
  }

  void consume() {
    (void)get_token();
  }
  bool skip(ETokenType expected) {
    Token  t = get_token();
    if (t.type != expected)
    {
      error_at(t,expected);
      return false;
    }
    return true;
  }
  Token expect(ETokenType expected) {
    const Token  t = get_token();
    if (t.type != expected)
    {
      error_at(t,expected);
    }
    return t;
  }
};
