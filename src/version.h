#pragma once



#define HAIC_LANG_VERSION_MAJOR 0
#define HAIC_LANG_VERSION_MINOR 1



/* changelog

TODO: support unary-plus or not ?
TODO: support parse_Literal_value
TODO: support the error recover by Panic Mode（恐慌模式）

version-0.1.4:
  impl fn_call_expr : fn1(); fn2(arg1,arg2,...);
  impl struct_dot_expr,impl arr_expr
  add constexpr-flag "FLAG_STRICT_MODE" in lexer.h at StrLit/CharLit

version-0.1.3:
  FIX: isspace,isdigit,isalpha,isalnum : if c is utf-8 but not ascii(not in -1 or 0~255), will error
    replace above with is_space,is_digit,is_alpha,is_alnum
  add struct Expr (Pratt Parsing Expression)
  TODO: which better? struct Expr use unique_ptr or use raw_ptr + arena ? or unique_ptr + arena ??
    EXPR_VERSION 1 : unique_ptr + default new(Expr::operator new)
    EXPR_VERSION 2 : raw_ptr + arena(placement new)
      (now switch version by macro "EXPR_VERSION" "MAKE_PTR" "MOVE")(default version-2)

version-0.1.2:
  more details for error => add Token.line Token.column, Lexer.base Lexer.row
  Lexer: **basic** support StrLit and CharLit, remove '\'','"' from Puncts (Operators)
    TODO: support Escape Character-'\<.>'
    TODO: how to deal with the lack of Right-'"'-'\'' ?
      now default: when lack of Right-'"'-'\'' , will **auto add** Right-'"'-'\'' **at the end of line**;
    TODO: if the char_len != 1 in CharLit ? '' 'xy' '\\' '\0'
      now Strict Mode: will error; not-Strict Mode: same as '"',
  more Precedence(binding power) in kRules

version-0.1.1:
  FIX : after parse_*,directly delete root(: AstNode*),will leak the memory
    use unique_ptr instead of raw ptr

  support option --raw for fast testing

version-0.1:
  support basic keywords: (only lexer)
    s8 u8 s16 u16
    s32 u32 s64 u64
    f32 f64 string
    bool void false true
    struct var const fn
    if while break continue return

  support ops:
    unary ops: - ()
    binary ops: + - * / =

  support comment:
    single-comment "//"


*/
