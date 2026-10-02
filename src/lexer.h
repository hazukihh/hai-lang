#pragma once
#include "common/Log.h"
#include "common/Assert.h"
#include "common/StringView.h"

#include "tokens_def.h"



// void parser_error_at(const Token& t,StringView msg,bool panic = true);
// void parer_error_at(const Token& t,ETokenType expected,bool panic = true);

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
  bool enable_lexer_report_error = true;
  //
  // Parser
  //
  Token buf_[2] = {{ETK_None},{ETK_None}};
  uint8_t buf_size_= 0;

  // TODO: should it be moved to a new Class Parser?
  bool isError = false;
  bool isPanic = false;
  //
  // Parser End
  //

  bool init(std::string_view src);

  void error_at(const Token& t, StringView msg,bool panic = true);
  void error_at(const Token& t, ETokenType expected,bool panic = true);
  void sync(ETokenType expected);

  [[nodiscard]] Token get_token();

  // template<bool print_error = true>
  [[nodiscard]] Token next();


  void consume();

  [[nodiscard]]
  const Token& peek() ;

  [[nodiscard]]
  const Token& peek1() ;


  // 可选匹配（未匹配不报错）
  [[nodiscard]] bool is_match(ETokenType expected);

  Token expect(ETokenType expected);

private:
  Token get_token_skip_unknown();
  void debug_check_text_loc_valid(const Token& t) const;
};
