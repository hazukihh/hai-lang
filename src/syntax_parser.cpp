#include "syntax_parser.h"

#include "common/StringView.h"
#include "common/variant_match.h"
#include "lexer.h"


void Stmt::print(int depth /* = 0*/)
{
  auto get_value = [](const Token& t)
  {
    return std::string(t.to_str());
  };
  // MATCH(this->data,
    std::visit(Overloaded{
    [get_value,depth](const Expr_Stmt& stmt){
      LOG_INFO("--Expr_stmt--");
      expr_print(stmt.expr,get_value,depth);
    },
    // [](const Null_Stmt&)
    // {
    //   LOG_INFO("--Null_Stmt--");
    // },
    [get_value,depth](const Ret_Stmt& stmt)
    {
      LOG_INFO("----Return----");
      expr_print(stmt.val,get_value,depth);
    },
    [depth](const Block_Stmt& stmt)
    {
      LOG_INFO("{");
      for (size_t i =0;i<stmt.cnt;++i)
      {
        // LOG_INFO("{:{}}--{}","",4,i+1);
        stmt.stmts[i]->print(depth + 1);
      }
      LOG_INFO("}");
    },
    [get_value,depth](const If_Stmt& stmt)
    {
      LOG_INFO("----If----");
      LOG_INFO(".cond:");
      expr_print(stmt.cond,get_value,depth + 1);
      LOG_INFO(".then");
      stmt.then->print(depth+1);
      if (stmt.els)
      {
        LOG_INFO(".else");
        stmt.els->print(depth+1);
      }
    },
    [get_value,depth](const While_Stmt& stmt)
    {
      LOG_INFO("----While----");
      LOG_INFO(".cond:");
      expr_print(stmt.cond,get_value,depth+1);
      LOG_INFO(".body:");
      stmt.body->print(depth+1);
    },
    [depth](const Break_Stmt& stmt)
    {
      LOG_INFO("{:{}}{}","",depth*4,"break");
    },
    [depth](const Continue_Stmt& stmt)
    {
      LOG_INFO("{:{}}{}","",depth*4,"continue");
    },
    [get_value](const Var_Decl_Stmt& stmt)
    {
      LOG_INFO("----Var\n"
      "  .name = {}\n"
      "  .type = {}\n"
      "  .init =\n",
      stmt.name.text,stmt.type->to_str());
      expr_print(stmt.init,get_value,2);
    },
    [depth](const Fn_Decl_Stmt& stmt)
    {
      LOG_INFO(
        "----Fn\n"
        "{}(",
        stmt.name.text
        );
      fmt::memory_buffer buf;
      for (int i =0;i<stmt.params_cnt;++i)
      {
        if (i!=0) buf.append(sv_from_lit(",\n"));
        fmt::format_to(std::back_inserter(buf),
          "{}: {}",
          stmt.params[i].name.text,stmt.params[i].type->to_str());
      }
      LOG_INFO(fmt::to_string(buf));
      LOG_INFO(") -> {}",stmt.ret_type->to_str());
      stmt.body->print(depth+1);
    },
    // [](const auto& stmt)
    [](const std::monostate&)
    {
      LOG_INFO("INVALID_STMT");
    }
      },this->data
  );
}


// ==============
// Internal Decl
// ==============


/**
 * expr_stmt / null_stmt ::= expr? ";"
 */
static Stmt::Ptr parse_expr_stmt(Lexer *lexer);

/**
 * ret_stmt ::= "return" expr? ";"
 */
static Stmt::Ptr parse_ret_stmt(Lexer *lexer);

/**
 * block_stmt ::= "{" stmt* "}"
 */
static Stmt::Ptr parse_block_stmt(Lexer *lexer);

/**
 * if_stmt ::= "if" expr block_stmt ["else" block_stmt | "else" if_stmt]
 */
static Stmt::Ptr parse_if_stmt(Lexer *lexer);

/**
 * while_stmt ::= "while" expr block_stmt
 */
static Stmt::Ptr parse_while_stmt(Lexer *lexer);

/**
 * break_stmt ::= "break" ";"
 */
static Stmt::Ptr parse_break_stmt(Lexer *lexer);
/**
 * continue_stmt ::= "continue" ";"
 */
static Stmt::Ptr parse_continue_stmt(Lexer *lexer);

static bool IsKeywordBaseType(ETokenType e)
{
  // TODO: assert({s8 u8 ...  string bool void} is consecutive)
  HAI_ASSERT(ETK_s8 + 1 == ETK_u8);

  return ETK_s8 <= e && e <= ETK_void;
}
static TypeInfo* parse_type(Lexer* lexer);

/**
  var_decl_stmt  => "var" <identifier> [[":" [<Type>] ("=" | ":")] expr]";" // like jai
                 => [const] "var" <identifier> [[":"<Type>] "="expr]";"
                 => (const | "var") <identifier> [":"<Type>] ["=" expr] ";"
 */
static Stmt::Ptr parse_var_decl_stmt(Lexer *lexer);

static Stmt::Ptr parse_fn_decl_stmt(Lexer *lexer);


// =======
//  Impl
// =======

#define ARENA_NEW(type) ::new (static_cast<type*>(g_arena.alloc(sizeof(type)))) type

#define ARENA_NEW_ARR(type,cnt) ::new (static_cast<type*>(g_arena.alloc(sizeof(type) * cnt))) type[cnt]

/**
 * expr_stmt / null_stmt ::= expr? ";"
 */
Stmt::Ptr parse_expr_stmt(Lexer* lexer)
{

  if (lexer->peek().type == ETK_Semi)
  {
    lexer->consume();
    return ARENA_NEW(Stmt){Expr_Stmt{}};
  }

  auto left = parse_expression(lexer,0);
  lexer->skip(ETK_Semi);


  return ARENA_NEW(Stmt){Expr_Stmt{left}};
}
/**
 * ret_stmt ::= "return" expr? ";"
 */
Stmt::Ptr parse_ret_stmt(Lexer* lexer)
{
  lexer->skip(ETK_return);

  return parse_expr_stmt(lexer);
}

/**
 * block_stmt ::= "{" stmt* "}"
 */
Stmt::Ptr parse_block_stmt(Lexer* lexer)
{

  lexer->skip(ETK_LBrace);

  std::vector<Stmt*> stmts;

  while (lexer->peek().type != ETK_RBrace)
  {
    stmts.emplace_back(parse_stmt(lexer));
  }
  lexer->skip(ETK_RBrace);


  Stmt **arr = nullptr;
  if (!stmts.empty())
  {
    const size_t size = sizeof(Stmt*) * stmts.size();
    arr = static_cast<Stmt**>(g_arena.alloc(size));

    std::memcpy(arr,stmts.data(),size);
  }
  return ARENA_NEW(Stmt){Block_Stmt{
    .stmts = arr,
    .cnt = stmts.size()
  }};
}

/**
 * if_stmt ::= "if" expr block_stmt ["else" block_stmt | "else" if_stmt]
 */
Stmt::Ptr parse_if_stmt(Lexer* lexer)
{
  If_Stmt result;

  lexer->skip(ETK_if);
  result.cond = parse_expression(lexer);
  result.then = parse_block_stmt(lexer);

  if (lexer->peek().type == ETK_else)
  {
    lexer->consume();

    result.els = (lexer->peek().type == ETK_LBrace)
      ? parse_block_stmt(lexer)
      : parse_if_stmt(lexer);
  }

  return ARENA_NEW(Stmt){std::move(result)};
}

/**
 * while_stmt ::= "while" expr block_stmt
 */
Stmt::Ptr parse_while_stmt(Lexer* lexer)
{
  While_Stmt result;

  lexer->skip(ETK_while);
  result.cond = parse_expression(lexer);
  result.body = parse_block_stmt(lexer);

  return ARENA_NEW(Stmt){std::move(result)};
}
/**
 * break_stmt ::= "break" ";"
 */
Stmt::Ptr parse_break_stmt(Lexer* lexer)
{
  Stmt::Ptr result =  ARENA_NEW(Stmt)(Break_Stmt{
    lexer->expect(ETK_break)});
  lexer->skip(ETK_Semi);

  return result;
}

/**
 * continue_stmt ::= "continue" ";"
 */
Stmt::Ptr parse_continue_stmt(Lexer* lexer)
{
  Stmt::Ptr result = ARENA_NEW(Stmt)(Continue_Stmt{
    lexer->expect(ETK_continue)
  });
  lexer->skip(ETK_Semi);

  return result;
}
template<typename Int>
Int parse_number_int(StringView text)
{
  Int val;
  auto result = std::from_chars(
        text.data(),text.data()+text.size(),
        val
        );
  HAI_ASSERT(result.ec == std::errc{}
    && result.ptr == text.data()+text.size()
    && "parse IntLit failed");
  return val;
}

TypeInfo* parse_type(Lexer* lexer)
{
  TypeInfo* info = TypeInfo::Create();

  auto t = lexer->next();
  switch (t.type){
  case ETK_Star: {
    info->tag = ETypeTag::ETy_Pointer;
    info->pointer.base = parse_type(lexer);
  }break;
  case ETK_LBracket: {
    info->tag = ETypeTag::ETy_Array;

    auto next_tok = lexer->next();
    if (next_tok.type == ETK_RBracket)
    {
      info->arr.size = 0;
    } else if (next_tok.type == ETK_IntLit){
      info->arr.size = parse_number_int<size_t>(next_tok.text);
      lexer->skip(ETK_RBracket);
    } else {
      HAI_ASSERT(false && "unreachable or todo ETK_Dot3");
      lexer->skip(ETK_RBracket);
    }

    info->arr.base = parse_type(lexer);
  }break;
  case ETK_LParen: {
    info->tag = ETypeTag::ETy_Function;
    HAI_ASSERT(false && "fn type parsing TODO");
  }break;
  case ETK_Identifier: {
    info->tag = ETypeTag::ETy_Named;
    info->tok = t;
  }break;
  default: {
    if (!IsKeywordBaseType(t.type)) {
      error_at(t,sv_from_lit("expected Type"));
    }
    info->tag = ETypeTag::ETy_Builtin;
    info->tok = t;
  }break;
  }

  // TODO: temp : Type*
  return info;
}

/**
  var_decl_stmt (temp now)::= "var" <identifier> [":"<Type>] ["=" expr] ";"
 */
Stmt::Ptr parse_var_decl_stmt(Lexer* lexer)
{
  Var_Decl_Stmt result;
  lexer->skip(ETK_var);

  // TODO: Symbol Table and String Pool ?
  result.name = lexer->expect(ETK_Identifier);

  /// Optional : type decl
  if (lexer->peek().type == ETK_Colon)
  {
    lexer->consume();

    result.type = parse_type(lexer);
  }

  /// Optional : init with expr
  if (lexer->peek().type == ETK_Assign)
  {
    lexer->consume();
    result.init = parse_expression(lexer);

  }
  lexer->skip(ETK_Semi);

  return ARENA_NEW(Stmt){std::move(result)};
}


/**
  fn_decl_stmt ::= fn <identifier> "(" <param_arr>? ")" "->" <ret_type> block_stmt
  param_arr ::= param ["," param_arr]
  param ::= <Ident> ":" <Type>
 */
Stmt::Ptr parse_fn_decl_stmt(Lexer* lexer)
{
  Fn_Decl_Stmt result;
  lexer->skip(ETK_fn);
  result.name = lexer->expect(ETK_Identifier);

  std::vector<ParamInfo> arr;

  auto parse_param = [lexer,&arr](){
    ParamInfo info;
    info.name = lexer->expect(ETK_Identifier);
    lexer->skip(ETK_Colon);
    info.type = parse_type(lexer);

    arr.emplace_back(std::move(info));
  };

  lexer->skip(ETK_LParen);
  if (lexer->peek().type != ETK_RParen)
  {
    parse_param();
    while (lexer->peek().type == ETK_Comma)
    {
      lexer->consume();
      parse_param();
    }
  }
  lexer->skip(ETK_RParen);

  ParamInfo* params = nullptr;
  if (!arr.empty())
  {
    params = static_cast<ParamInfo*>(g_arena.alloc(sizeof(ParamInfo) * arr.size()));
    for (int i=0;i<arr.size();++i)
    {
      params[i] = std::move(arr[i]);
    }
  }
  result.params = params;
  result.params_cnt = arr.size();

  lexer->skip(ETK_RArrow);
  result.ret_type = parse_type(lexer);

  result.body = parse_block_stmt(lexer);

  return ARENA_NEW(Stmt){std::move(result)};
}


Stmt::Ptr parse_stmt(Lexer* lexer)
{
  switch (lexer->peek().type) {
  case ETK_return:   return parse_ret_stmt(lexer);
  case ETK_LBrace:   return parse_block_stmt(lexer);
  case ETK_if:       return parse_if_stmt(lexer);
  case ETK_while:    return parse_while_stmt(lexer);
  case ETK_break:    return parse_break_stmt(lexer);
  case ETK_continue: return parse_continue_stmt(lexer);
  case ETK_var:      return parse_var_decl_stmt(lexer);
  case ETK_fn:       return parse_fn_decl_stmt(lexer);
  default:           return parse_expr_stmt(lexer);
  }

  HAI_ASSERT(false && "parse_stmt unreachable");
}
