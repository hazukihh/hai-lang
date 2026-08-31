#pragma once



#define HAIC_LANG_VERSION_MAJOR 0
#define HAIC_LANG_VERSION_MINOR 1



/* changelog

TODO: support unary-plus or not ?
TODO: support the error recover by Panic Mode（恐慌模式）
TODO: support lit_suffix in parse_expr() ?
TODO: type.h,symbol_table.h,sema.h; syntax_parser.h -> parser.h


version-0.1.5:
  FIX: silly bug at "expr.cpp".infix_call(...);
    Value &p same as Value * const p,, p = ...,can't change target of the pointer, will change the value;
  FIX: arena.h alloc()
    void* ptr = pool+ size_ + size;size_ += size;  => void* ptr = pool+ size_;size_ += size;

  StringView.h add fn str_from_lit(str_literal) -> StringView;

  optimize the struct Expr, let it better store the data/value (XXXLiteral,CallExpr,...), use variant or TaggedUnion
    TODO: how to store parse result of literal: use BigInt+BigFloat or uint64_t+long double ?
      (should or how) to deal with overflow (value > uint64_MAX) ??
          ???
          BigInt: class llvm::APInt {
            // 动态分配或固定64位存储
            uint64_t* pVal;   // 指向数值存储
            unsigned bitWidth; // 位宽（如 32, 64, 128...）
            bool isSigned;    // 符号标记！
          }; ???

  impl Basic Stmt and Basic decl: @return @block_Stmt @if @while @break @continue @var_decl @fn_decl
    TODO: IdentInfo?? (now just StringView)
  impl Basic Type parse System: TypeInfo
    TODO: parse_type(): support parsing fn_type like ()->void ... (a1,a2,a3) -> T;
    TODO: impl Type* and TypePool? (now just StringView for Builtin and Named)

  replace assert() with custom assert "HAI_ASSERT()"
  replace std::unordered_map with phmap::flat_hash_map (lib parallel-hashmap)


version-0.1.4:
  impl fn_call_expr : fn1(); fn2(arg1,arg2,...);
  impl struct_dot_expr,impl arr_expr
  add constexpr-flag "FLAG_STRICT_MODE" in lexer.h for StrLit/CharLit

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
    TODO: support ALL Escape Character-'\<.>';
    FIXME: support detect \ddd:三位八进制,\xhh:二位十六进制
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
