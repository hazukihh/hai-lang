#pragma once
#include "common/Log.h"
#include "common/Assert.h"
#include "common/StringView.h"

#include "tokens_def.h"

// TODO: Option for Strict Mode
constexpr auto FLAG_STRICT_MODE = true;

inline void error_at(const Token& t,StringView msg) {

  auto column = t.loc.column(t.text.data());


  LOG_ERROR(
    "at line {}({}): got \"{}\":\"{}\", {}\n"
    "| {}\n"
    "| {:{}}^",
    t.loc.line, column, t.to_str(),get_type_tag_str(t.type),msg,
    StringView{t.loc.line_start,column + t.text.size()},
    "",column
  );


  // TODO: how to continue to check; temp exit
  HAI_ASSERT(false);
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

  // lexer.src_ : debug_assert Token.text is in the range of src_ : debug_check_text_loc_valid(t.text);
#ifndef NDEBUG
  std::string_view src_;
#endif

  bool init(std::string_view src)
  {
#ifndef NDEBUG
    src_ = src;
#endif
    input_ = src;
    base_ = input_.data();
    return true;
  }

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
        while (i < size && is_space(data[i])) {
          if (data[i] == '\n') {
            row_ += 1;
            base_ = data + i + 1;
          }

          i += 1;
        }
        input_ =  StringView{data + i, size - i};
      }

      if(sv_starts_with(input_,SlComment)) {

        // Chop until the end of line
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
        .loc = {
          .line = row_,
          .line_start = base_
        }
      };
    }

    // Puncts
    HAI_ASSERT(kEStr_Puncts[0][1] != '\0' && "should check the 2-char ops first");
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
          .loc = {
            .line = row_,
            .line_start = base_
          }
        };
      }
    }

    // Int literal
    // TODO: hex: 0X/0x...; Oct 0O/0o...
    // TODO: float literal
    if(is_digit(input_[0])) {
      size_t pos = 0;
      while(pos < input_.size() && is_digit(input_[pos])) {
        ++pos;
      }
      StringView literal_sv = {input_.data(),pos};
      input_ = sv_slice(input_,pos);

      return Token{
        .type = ETK_IntLit,
        .text = literal_sv,
        .loc = {
          .line = row_,
          .line_start = base_
        }
      };
    }

    /**
     * @note how to deal with Conflict in Punct '\"' '\''
     *    probably not has operator for '\"' '\'' , So just remove them from puncts
     */
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

        if constexpr (FLAG_STRICT_MODE) {
          error_at(Token{
            .type = ETK_StrLit,
            .text = sv,
            .loc = {
              .line = row_,
              .line_start = base_
            }
          },"StringLiteral lack Right-'\"'(Strict Mode)");
        }
      }
      return Token{
        .type = ETK_StrLit,
        .text = sv,
        .loc = {
          .line = row_,
          .line_start = base_
        }
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


      StringView sv;
      if (pos < input_.size() && input_[pos] == '\'') {
        sv = StringView{input_.data(),pos+1};
        input_ = sv_slice(input_,pos+1);
      } else {
        LOG_WARN("CharLiteral lack Right-'\''");
        sv = StringView{input_.data(),pos};
        input_ = sv_slice(input_,pos);

        if constexpr (FLAG_STRICT_MODE) {
          error_at(Token{
            .type = ETK_CharLit,
            .text = sv,
            .loc = {
              .line = row_,
              .line_start = base_
            }
          },"CharLiteral lack Right-'\''(Strict Mode)");
        }
      }
      // TODO: Similar to StringLit. Should be error when the char len > 1 ? or delay ?
      if constexpr (FLAG_STRICT_MODE) {
        // FIXME: support detect \ddd:三位八进制,\xhh:二位十六进制
        if(sv.size()<3 ||
          (sv[1]!='\\' && sv.size()!=3) ||
          (sv[1]=='\\' && sv.size()!=4))
        {
          error_at(Token{
            .type = ETK_CharLit,
            .text = sv,
            .loc = {
              .line = row_,
              .line_start = base_
            }
          },"CharLiteral must be single char");
        }
      }
      return Token{
        .type = ETK_CharLit,
        .text = sv,
        .loc = {
          .line = row_,
          .line_start = base_
        }
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
          .loc = {
            .line = row_,
            .line_start = base_
          }
        };
      }


      // Identifier
      return Token{
        .type = ETK_Identifier,
        .text = sv,
        .loc = {
          .line = row_,
          .line_start = base_
        }
      };
    }

    // Unknown character
    LOG_WARN("Unknown character: {}", input_[0]);
    Token unknown = {
      .type = ETK_None,
      .text = sv_slice(input_,0,1),
      .loc = {
        .line = row_,
        .line_start = base_
      }
    };
    input_ = sv_slice(input_,1);
    return unknown;
  }

  [[nodiscard]]
  Token peek() {
    Lexer lexer_copy = *this;
    auto t = lexer_copy.get_token();
    debug_check_text_loc_valid(t);
    return t;
  }

  [[nodiscard]]
  Token next() {
    auto t = get_token();
    debug_check_text_loc_valid(t);
    return t;
  }

  void consume() {
    auto t = get_token();
    debug_check_text_loc_valid(t);
  }

  bool skip(ETokenType expected) {
    Token  t = get_token();
    debug_check_text_loc_valid(t);
    if (t.type != expected)
    {
      error_at(t,expected);
      return false;
    }
    return true;
  }
  Token expect(ETokenType expected) {
    const Token  t = get_token();
    debug_check_text_loc_valid(t);
    if (t.type != expected)
    {
      error_at(t,expected);
    }
    return t;
  }
private:
  void debug_check_text_loc_valid(const Token& t) const
  {
#ifndef NDEBUG
    auto* line_start = t.loc.line_start;
    auto* start = t.text.data();
    auto* end = t.text.data() + t.text.size();
    HAI_ASSERT(src_.data() <= line_start
      && line_start <= start
      && start <= end
      && end <= src_.data() + src_.size()
      && "token.text or .loc.line_start out of range of string_view src");
#endif
  }
};
