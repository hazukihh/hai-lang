#pragma once



#define HAIC_LANG_VERSION_MAJOR 0
#define HAIC_LANG_VERSION_MINOR 1



/* changelog
 *
TODO: more details for error => token.line .column, lexer.row..
TODO: support unary-plus or not ?
TODO: support the error recover by Panic Mode（恐慌模式）



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
