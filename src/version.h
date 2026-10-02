#pragma once



#define HAIC_LANG_VERSION_MAJOR 0
#define HAIC_LANG_VERSION_MINOR 1



/* changelog

TODO: support unary-plus or not ?
TODO: TokenLoc better solution for error report:
	(now)(1) int line;const char* line_start; 提前计算line值,column值延迟到错误时，结合text_start `:*const char` 计算
	(2) "一些标记实现将位置存储为两个数字：从源文件开始到词素开始的偏移量，以及词素的长度。扫描器无论如何都会知道这些数字，因此计算这些数字没有任何开销。通过回头查看源文件并计算前面的换行数，可以将偏移量转换为行和列位置。这听起来很慢，确实如此。然而，只有当你需要向用户实际显示行和列的时候，你才需要这样做。大多数标记从来不会出现在错误信息中。对于这些标记，你花在提前计算位置信息上的时间越少越好。" => 将line值也延迟到错误发生时计算，从源文件开头src `:* const char`开始扫描计算

TODO: support lit_prefix, lit_suffix in parse_expr() ? or in Lexer ?

TODO: type.h,symbol_table.h,sema.h;


version-0.1.7:
  (1)StringView.h  add functions "sv_trim_*_if"
  (2)FIX: lexer: ('\'') should be correct, but ('\') should be error
  (3)impl parse_StringLit, parse_charLit :
    Escape Character Syntax:
      \a  alert (bell)
      \b  backspace
      \f  form feed
      \n  new line
      \r  carriage return
      \t  horizontal tab
      \v  vertical tab
      \\  backslash
      \'  single quote
      \"  double quote
      don't support \?  question mark
      \e  escape character (ASCII 27) [GNU C extension]
      \0  null character (ASCII 0)
      don't support \ddd  octal number (1~3 octal digits)
      \xhh  hex number (**fixed 2** hex digits)
      TODO: support \uXXXX \UXXXXXXXX or \u{...} for unicode char or
      TODO: support \d123, max \d255
  (3.1) the StringLiteral use the string_arena-string-pool to store.(use flat_hash_set to index the string);
  (4) replace my-"arena.hpp" with "tsoding-arena.h".
    The C++ Wrapper of "tsoding-arena.h": "arena.h" + "arena.cpp" + internal/arena

  (5)TODO: support Fn_Decl ： when fn name(...)->void ，can become fn name(...);
   And "var func: fn(...)" is error, must be "var func: fn(...)->void"
    TODO:??? how to re-use the same TypeInfo? Not Create a new one;But, now the TypeInfo has TypeInfo.tok
      so, "fn name(...)" don't has token for "void".



version-0.1.6:
  (1)impl BasicTypeSystem.TypeInfo.<fn_type>
    "fn()->void" ,"fn(t1,t2,...)->void" ,"fn(a1:t1,a2:t2,...)->void"
  (2)impl Multi_Comment in lexer.h, Supports nested comments
  (3)impl the PanicMode for the ErrorPrint in Lexer(Parser)
    TODO: haven't test all
  (4.1)Lexer when StrLit or CharLit lack of Right-'"'-'\''.
    immediately PrintError(),
    and ErrorRecover: From Left-'"'-'\'' to (\r\n | \n) are treated as Literal.
    Then Set isPanic false.
  (4.2)Lexer don't check  the char_len == 1 in CharLit (examples: '' 'xy' '\\' '\0' '\\\')
    the CharLit will same as StrLit, the check delay to The-Step-Sema
  (5)Optimize: the Lexer(Parser) cache two Token in buf_.(used with buf_size_)
    next(),peek(),peek1() are get token from buf_. when needed, call get_token() and cache Token to buf_

  (6)FIX: forget to add prefix_unary_op for address-of-"&" and ( unref-"*" or ".*")
    TODO: which better? use *p or p.* ?
      temp not impl prefix_unary_op for unref

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
    TO[0.1.6#1]DO: parse_type(): support parsing fn_type like ()->void ... (a1,a2,a3) -> T;
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
  TO[0.1.7#4]DO: which better? struct Expr use unique_ptr or use raw_ptr + arena ? or unique_ptr + arena ??
    EXPR_VERSION 1 : unique_ptr + default new(Expr::operator new)
    EXPR_VERSION 2 : raw_ptr + arena(placement new)
      (now switch version by macro "EXPR_VERSION" "MAKE_PTR" "MOVE")(default version-2)

version-0.1.2:
  more details for error => add Token.line Token.column, Lexer.base Lexer.row
  Lexer: **basic** support StrLit and CharLit, remove '\'','"' from Puncts (Operators)
    TO[0.1.7#3]DO: support ALL Escape Character-'\<.>';
      \ddd:三位八进制,\xhh:二位十六进制
    TO[0.1.6#4.1]DO: how to deal with the lack of Right-'"'-'\'' ?
      now default: when lack of Right-'"'-'\'' , will **auto add** Right-'"'-'\'' **at the end of line**;
    TO[0.1.6#4.2]DO: if the char_len != 1 in CharLit ? '' 'xy' '\\' '\0'
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
