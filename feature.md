Lexer:
1. 遇到非法字符(ETK_None)时, 会报告错误, 但在后续解析中忽略这些字符产生的Token (get_token_skip_unknown)
2.



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
    bool,void
    string /*TODO: should builtin the type string ?*/
    false, true
    struct, var
    const, fn
    if,else
    while,for
    break,continue
    return
  Fn Type:
    fn (n1:t1,n2:t2,...) -> ret_t
    fn (t1,t2,...) -> ret_t
  Pointer:
    *T
  Array:
    []T
    [N]T
3. Literal
  CharLiteral '.'   : u32
  StringLiteral "*" : [N] const u8
  
  
4. Fn Decl:
  fn <fn_name> (p1:t1, p2:t2, ...) -> ret_t {...}
  fn <fn_name> (p1:t1, p2:t2, ...) {...}
    same as fn <fn_name> (p1:t1, p2:t2, ...) -> void {...}
    
5. TODO:?? for ??
  // TODO:jai
  
  Don't support??
    for expr? ; expr ; expr? {...}
  
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