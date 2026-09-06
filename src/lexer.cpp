#include "lexer.h"

#if 0
// TODO: ??new Class Parser?
struct Parser
{
  Token previous;
  Token current;
  bool isError = false;
  bool isPanic = false;
};

void error_at(const Token& t, StringView msg)
{
  if (isPanic) return;
  isPanic = true;

  auto column = t.loc.column(t.text.data());


  LOG_ERROR(
    "at line {}({}): got \"{}\":\"{}\", {}\n"
    "| {}\n"
    "| {:{}}^",
    t.loc.line, column, t.text,get_type_tag_str(t.type),msg,
    StringView{t.loc.line_start,column + t.text.size()},
    "",column
  );


  // TODO: how to continue to check; temp exit
  // HAI_ASSERT(false);

  isError = true;
}

void error_at(const Token& t, ETokenType expected)
{
  error_at(t,fmt::format("expected \"{}\"",get_type_tag_str(expected)));
}
#endif
//
// Struct Lexer
//

static int utf8_char_len(unsigned char c)
{
  if (c < 0x80) return 1;
  if ((c >> 5) == 0x06) return 2;
  if ((c >> 4) == 0x0E) return 3;
  if ((c >> 3) == 0x1E) return 4;
  return 1; // 非法 UTF-8 前导字节，按 1 字节处理防死循环
}


bool Lexer::init(std::string_view src)
{
#ifndef NDEBUG
  src_ = src;
#endif
  input_ = src;
  base_ = input_.data();
  return true;
}

void Lexer::error_at(const Token& t, StringView msg)
{
  if (this->isPanic) return;
  isPanic = true;
  this->isError = true;

  auto column = t.loc.column(t.text.data());
  bool single = t.text.size() < 2;
  LOG_ERROR(
    "at line {}({}): got ({}): {}, {}\n"
    "| {}\n"
    "  {:{}}^{:{}}{}",
    t.loc.line, column, t.text,ETokenType_to_Str(t.type),msg,
    StringView{t.loc.line_start,column + t.text.size()},
    "",column,"",single ? 0 : t.text.size()-2,single ?"":"^"
  );

}

void Lexer::error_at(const Token& t, ETokenType expected)
{
  error_at(t,fmt::format("expected \"{}\"",ETokenType_to_Str(expected)));
}

void Lexer::sync(ETokenType expected)
{
  if (!this->isPanic) return;
  LOG_DEBUG("[Parser] panic mode sync");
  this->isPanic = false;

  auto t = this->peek();
  Token range[2];
  uint8_t cnt = 0;
  for (bool loop=true;loop; t= this->peek())
  {
    switch (t.type){
    case ETK_EOF: case ETK_RBrace:
    case ETK_if: case ETK_while: case ETK_return:
    case ETK_var: case ETK_fn: case ETK_struct:
      loop = false;
      break;
    case ETK_Semi:
      loop = false;
      if (cnt == 0)
      {
        range[0] = this->next();
        range[1] = range[0];
        ++cnt;
      } else {
        range[1] = this->next();
      }

      break;
    default:
      if (cnt == 0)
      {
        range[0] = this->next();
        range[1] = range[0];
        ++cnt;
      } else {
        range[1] = this->next();
      }
      break;
    }
  }
  if (cnt != 0)
  {
    range[0].text = StringView{range[0].text.data(),range[1].text.data() + range[1].text.size()};
    this->error_at(range[0],"Ignore Codes");
    this->isPanic =false;
  }
}

Token Lexer::get_token()
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

    // MlComment-Left-Right
    if (sv_starts_with(input_,LeftMlComment))
    {
      input_ = sv_slice(input_,LeftMlComment.size());
      int comment_depth = 1;

      while (comment_depth && !input_.empty())
      {
        const size_t pos = input_.find_first_of(MlCommentCharSet_AND_LF);
        if (pos == StringView::npos) [[unlikely]] {
          input_ = sv_slice(input_, input_.size());
          break;
        }

        if (input_[pos] == '\n')
        {
          input_ = sv_slice(input_,pos + 1);
          row_ += 1;
          base_ = input_.data();
          continue;
        }

        input_ = sv_slice(input_,pos);
        if (sv_starts_with(input_,LeftMlComment))
        {
          input_ = sv_slice(input_,LeftMlComment.size());
          comment_depth +=1;
        }
        else if (sv_starts_with(input_,RightMlComment))
        {
          input_ = sv_slice(input_,RightMlComment.size());
          comment_depth -=1;
          if (comment_depth == 0) {
            break;
          }
        }
        else
        {
          input_ = sv_slice(input_,1);
        }
      }
      continue;
    }

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
  /**
   * TODO：how to optimize from O(n) to O(1)
   *    ？先hash检查是否是one char ops,如果是就检查下一个字符，是否可以2 char ops
   *  TODO: which better? use gpref to generate the keyword set(perfect hash) to match
   *    or use trie-tree(switch-case) to match
   */
  for(int i = ETK_PUNCT_START; i < ETK_PUNCT_END; ++i) {
    auto punct_str = StringView{GetPunctCStr(static_cast<ETokenType>(i))};

    if(sv_starts_with(input_,punct_str)) {

      StringView sv = sv_slice(input_,0,punct_str.size());
      input_ = sv_slice(input_,punct_str.size());

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
   * TODO: operator ""_suffix() {}
   */
  /// String literal "..."
  if (input_[0] == '"')
  {
    size_t pos = 1;
    bool is_closed = false;

    while (pos < input_.size())
    {
      const unsigned char c = input_[pos];
      if (c == '"')
      {
        is_closed = true;
        break;
      }
      /// Feature: don't support Multi-Line-String
      if (c == '\n') break;

      ++pos;
    }

    // pos.* == '\"'        => input_[0:pos+1)
    // TODO: Tip: warning: lack Right-'\"', but assume there has one and correct. Or directly error here?
    // pos == input_.size() => input_[0:pos+1) => input_[0:size()+1) => input_[0:size()) => input_[0:pos)
    // pos.* == '\n'        => input_[0:pos) , leave the rest for next-get in order to support row_/base_
    StringView sv;
    if (is_closed) {
      sv = StringView{input_.data(),pos+1};
      input_ = sv_slice(input_,pos+1);
    }
    /// EOF or '\n'
    else {
      /// Error Recover: chop until the-end-of-line or EOF
      // LOG_WARN("StringLiteral lack Right-'\"'");
      size_t sv_end = (pos > 0 && input_[pos - 1] == '\r') ? pos-1:pos;
      sv = sv_slice(input_,0,sv_end);
      input_ = sv_slice(input_,pos);

      error_at(Token{
        .type = ETK_StrLit,
        .text = sv,
        .loc = {
          .line = row_,
          .line_start = base_
        }
      },"StringLiteral lack Right-'\"'");
      this->isPanic = false;

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

  /// Char literal '...'
  if (input_[0] == '\'')
  {
    size_t pos = 1;
    bool is_closed = false;

    while (pos < input_.size())
    {
      const unsigned char c = input_[pos];
      if (c == '\'')
      {
        is_closed = true;
        break;
      }
      /// Feature: don't support Multi-Line-String
      if (c == '\n') break;

      ++pos;
    }


    StringView sv;
    if (is_closed) {
      sv = StringView{input_.data(),pos+1};
      input_ = sv_slice(input_,pos+1);
    }
    // EOF or '\n'
    else
    {
      // LOG_WARN("CharLiteral lack Right-'\''");
      size_t sv_end = (pos > 0 && input_[pos - 1] == '\r') ? pos-1:pos;
      sv = sv_slice(input_, 0, sv_end );
      input_ = sv_slice(input_, pos);

      error_at(Token{
        .type = ETK_CharLit,
         .text = sv,
         .loc = {
           .line = row_,
           .line_start = base_
         }
      }, "CharLiteral lack Right-'\''");
      this->isPanic = false;
    }


    /// TODO: Similar to StringLit. Should be error when the char len > 1 ? or delay ?
    ///   Delay to parser
    return Token{
      .type = ETK_CharLit,
      .text = sv,
      .loc = {
        .line = row_,
        .line_start = base_
      }
    };
  }


  /// Identifier / Keyword
  if(IsSymbolStart(input_[0])) {

    size_t pos = 0;
    while(pos < input_.size() && IsSymbol(input_[pos])) {
      ++pos;
    }

    auto sv = StringView{input_.data(),pos};
    input_ = sv_slice(input_,pos);

    // Keyword
    ETokenType keyword_index = KeywordStr_to_ETokenType(sv);
    if ( keyword_index != ETK_Error)
    {
      HAI_ASSERT(ETK_KEYWORD_START <= keyword_index &&
      keyword_index < ETK_KEYWORD_END);

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
  int word_len = utf8_char_len(input_[0]);
  Token unknown = {
    .type = ETK_None,
    .text = sv_slice(input_,0,word_len),
    .loc = {
      .line = row_,
      .line_start = base_
    }
  };
  error_at(unknown, "Illegal character encountered");
  this->isPanic = false;

  input_ = sv_slice(input_,word_len);
  return unknown;
}

Token Lexer::next()
{
  if (buf_size_ == 0)
  {
    return get_token_skip_unknown();
  }
  else if (buf_size_ == 1)
  {
    --buf_size_;
    return buf_[0];
  }
  else if (buf_size_ == 2)
  {
    std::swap(buf_[0],buf_[1]);
    --buf_size_;
    return buf_[1];
  }
  HAI_UNREACHABLE();
  return {ETK_Error};
}

void Lexer::consume()
{
  (void)this->next();
}

const Token& Lexer::peek()
{
  if (buf_size_ == 0)
  {
    buf_[0] = get_token_skip_unknown();
    ++buf_size_;
    return buf_[0];
  }
  // buf_size_ >=1
  return buf_[0];
}
const Token& Lexer::peek1()
{
  while (buf_size_ < 2)
  {
    buf_[buf_size_] = get_token_skip_unknown();
    ++buf_size_;
  }
  return buf_[1];
}

bool Lexer::is_match(ETokenType expected)
{
/// FIX: should peek() not next();when success the consume();
  const auto&  t = this->peek();
  if (t.type == expected)
  {
    this->consume();
    return true;
  }

  return false;
}

Token Lexer::expect(ETokenType expected)
{
  /// FIX: should peek() not next();when success the consume();
  const Token&  t = this->peek();
  if (t.type == expected)
  {
    this->consume();
    return t;
  }

  this->error_at(t,expected);

  // TODO: expect() => immediately sync() error or not ???
  //    now: not needed
  // this->sync(expected);

  Token err_tok = t;
  err_tok.type = ETK_Error;
  return err_tok;
}

Token Lexer::get_token_skip_unknown()
{
  Token t;
  do
  {
    t = get_token();
  }while(t.type == ETK_None);
  return t;
}

void Lexer::debug_check_text_loc_valid(const Token& t) const
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

