Lexer:
1. 不支持utf8/unicode标识符:
  遇到非法字符(ETK_None)时, 会报告错误, 但在后续解析中会忽略这些字符产生的Token (get_token_skip_unknown)
  
2. Comment
  SingleLine        "//"
  MultiLine,可以嵌套 "/*/* */*/"
  
3. Identifier
  IdentStart   ::= [a-zA-Z_]
  IdentContent ::= [0-9a-zA-Z_]*
  Ident ::= IdentStart IdentContent
  
4. keywords
    s8 ,u8
    s16,u16
    s32,u32
    s64,u64
    f32,f64
    string
    bool,void
    false, true
    struct, var
    const, fn
    if,else
    while,for
    break,continue
    return


5. 不支持多行字符/字符串字面量,（"", '')必须在同一行,未闭合时报错
  
6. NumberLit 按 pp_number分词为Token，具体IntLit,FloatLit在parser处解析:
  0xFFe+2.0 0xFFp+2.0 123e 1.0_E+2.0 这些都会视为一个NumberLit Token
(而不是 0xFFe + 2.0 , ...)
  example parser result:
     Val      Suffix
    0xFFe      +2.0   error,invalid char '+''.' in suffix
    0xFFp+2.0    -    error,exp must be dec digits and not float
    123         e
    1.0_E+2.0         error,exp must be dec digits and not float


Parser and Sema

TODO: TypeInfo, Symbol_Table


Syntax:
1. Escape Char:
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
  Don't support \?  question mark
  \e  escape character (ASCII 27) [GNU C extension]
  \0  null character (ASCII 0)
  Don't support \ddd octal number (1~3 octal digits)
  \xhh  hex number (**fixed 2** hex digits)
  TODO: support \uXXXX \UXXXXXXXX or \u{...} for unicode char
  TODO: support \d123, and the maximum valid value is 255.
2. TypeInfo
  Builtin BaseType:
    s8 ,u8
    s16,u16
    s32,u32
    s64,u64
    f32,f64
    string /*TODO: should builtin the type string ?*/
    bool,void
  Fn Type:
    fn (n1:t1,n2:t2,...) -> ret_t
    fn (t1,t2,...) -> ret_t
  Pointer:
    *T
  Array:
    []T
    [N]T
3. Type for Literal
  CharLiteral '.'   : u32
  StringLiteral "*" : [N] const u8

4. Fn Decl:
  fn <fn_name> (p1:t1, p2:t2, ...) -> ret_t {...}
  (TODO)fn <fn_name> (p1:t1, p2:t2, ...) {...}
    same as fn <fn_name> (p1:t1, p2:t2, ...) -> void {...}

5. TODO: design for "for"

  Don't support??
    for expr? ; expr ; expr? {...}

  // learn form jai
  for ["@" IDENT ] [ ":" mode ] IDENT "in" (<Iterable> | <start> ".." "="? <end>) {}
    for i in start ".." "="? end {...}
    for iter in containter {}
    Named loop for "for", "while":
      while @cond x>3 {
        while true {
          continue cond;
          break cond;
        }
      }
      for @label i in 0..=5 {}
    // 使用编译期宏 还是 operator 重载
    // Jai #expand https://github.com/Jai-Community/Jai-Community-Library/wiki/Getting-Started#for_expansion
    // Jai for * p : array {}  for < i : 0..5 { // 5,4,3,2,1,0}
    fn operator dfs(FOR_BUILTIN_TAG,Containter c) -> Iter{
      ...
    }
    for @label :dfs iter in tree {}
    // builtin mode ":<" for Reverse Loop
    for :< i in 0..5 {}
    for containter {it_index; it;}

6. NumberLiteral syntax

Prefix ::= 0b | 0B | 0o | 0O | 0x | 0X
Digit ::= B_Digit | O_Digit | X_Digit | D_Digit
Album ::= [0-9a-zA-Z]
Suffix ::= (Album [Album_]*)
Hex_Lit   ::= Prefix Digit [Digit_]* (.[Digit__]*)? ([pP] [+-]? [0-9] [0-9_]*)? Suffix?
Dec_Lit   ::=        Digit [Digit_]* (.[Digit__]*)? ([eE] [+-]? [0-9] [0-9_]*)? Suffix?
Other_Lit ::= Prefix Digit [Digit_]* Suffix?

7. CharLit , StringLit ,StringLit support suffix
TODO
