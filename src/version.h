#pragma once



#define HAIC_LANG_VERSION_MAJOR 0
#define HAIC_LANG_VERSION_MINOR 1



/* changelog

TODO: support unary-plus or not ?
TODO: support the error recover by Panic Mode（恐慌模式）

version-0.1.2:
  more details for error => add Token.line Token.column, Lexer.base Lexer.row
  Lexer: **basic** support StrLit and CharLit, remove '\'','"' from Puncts (Operators)
    TODO: support Escape Character-'\...'
    TODO: how to deal with the lack of Right-'"'-'\'' ?
    TODO: if the char_len > 1 in CharLit ?
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
