#include "parser.h"

#include "common/StringView.h"
#include "common/variant_match.h"
#include "lexer.h"


void print(const Stmt* self,int depth /* = 0*/)
{
  if (!self)
  {
    LOG_INFO("{:{}}null-stmt","",depth * 4);
    return;
  }
  auto get_value = [](const Token& t)
  {
    return std::string(t.to_str());
  };
  MATCH(self->data,
  // std::visit(Overloaded {
      [get_value,depth](const Expr_Stmt& stmt){
        LOG_INFO("{:{}}--Expr_stmt--","",depth * 4);
        expr_print(stmt.expr,get_value,depth);
      },
      // [](const Null_Stmt&)
      // {
      //   LOG_INFO("--Null_Stmt--");
      // },
      [get_value,depth](const Ret_Stmt& stmt)
      {
        LOG_INFO("{:{}}----Return----","",depth * 4);
        expr_print(stmt.val,get_value,depth);
      },
      [depth](const Block_Stmt& stmt)
      {
        LOG_INFO("{:{}}{{","",depth * 4);
        for (size_t i =0;i<stmt.cnt;++i)
        {
          LOG_INFO("{:{}}--No.{}","",(depth+1) * 4,i+1);
          print(stmt.stmts[i],depth+1);
        }
        LOG_INFO("{:{}}}}","",depth * 4);
      },
      [get_value,depth](const If_Stmt& stmt)
      {
        LOG_INFO("{:{}}----If----","",depth * 4);
        LOG_INFO("{:{}}.cond:","",depth * 4);
          expr_print(stmt.cond,get_value,depth + 1);
        LOG_INFO("{:{}}.then","",depth * 4);
         print( stmt.then,depth+1);
        if (stmt.els)
        {
          LOG_INFO("{:{}}.else","",depth * 4);
            print(stmt.els,depth+1);
        }
      },
      [get_value,depth](const While_Stmt& stmt)
      {
        LOG_INFO("{:{}}----While----","",depth * 4);
        LOG_INFO("{:{}}.cond:","",depth * 4);
          expr_print(stmt.cond,get_value,depth+1);
        LOG_INFO("{:{}}.body:","",depth * 4);
          print(stmt.body,depth+1);
      },
      [depth](const Break_Stmt& stmt)
      {
        LOG_INFO("{:{}}break","",depth*4);
      },
      [depth](const Continue_Stmt& stmt)
      {
        LOG_INFO("{:{}}continue","",depth*4);
      },
      [get_value,depth](const Var_Decl_Stmt& stmt)
      {
        LOG_INFO("{0:{1}}----Var\n"
          "{0:{1}}  .name = {2}\n"
          "{0:{1}}  .type = {3}\n"
          "{0:{1}}  .init =",
          "",depth*4,
          stmt.name.text,stmt.type->to_str()
        );
        expr_print(stmt.init,get_value,depth+1);
      },
      [depth](const Fn_Decl_Stmt& stmt)
      {
        LOG_INFO("{0:{1}}----Fn\n"
          "{0:{1}}  .name = {2}\n"
          "{0:{1}}  .type = {3}\n"
          "{0:{1}}  .body =",
          "",depth*4,
          stmt.name.text,
          stmt.type->to_str()
        );
        print(stmt.body,depth+1);
      },
      // [](const auto& stmt)
      [](const std::monostate&)
      {
        LOG_INFO("INVALID_STMT");
      }
    // },self->data
    );
}

void print(const Stmt& self,int depth)
{
  print(&self,depth);
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

#define ARENA_NEW(type) ::new (g_arena.alloc<type>(1)) type

#define ARENA_NEW_ARR(type,cnt) ::new (g_arena.alloc<type>(cnt)) type[cnt]

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
  lexer->expect(ETK_Semi);


  return ARENA_NEW(Stmt){Expr_Stmt{left}};
}
/**
 * ret_stmt ::= "return" expr? ";"
 */
Stmt::Ptr parse_ret_stmt(Lexer* lexer)
{
  lexer->expect(ETK_return);

  return parse_expr_stmt(lexer);
}

/**
 * block_stmt ::= "{" stmt* "}"
 */
Stmt::Ptr parse_block_stmt(Lexer* lexer)
{

  lexer->expect(ETK_LBrace);

  std::vector<Stmt*> stmts;

  while (lexer->peek().type != ETK_RBrace)
  {
    stmts.emplace_back(parse_stmt(lexer));
  }
  lexer->expect(ETK_RBrace);


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

  lexer->expect(ETK_if);
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

  lexer->expect(ETK_while);
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
  lexer->expect(ETK_Semi);

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
  lexer->expect(ETK_Semi);

  return result;
}

/**
  var_decl_stmt (temp now)::= "var" <identifier> [":"<Type>] ["=" expr] ";"
 */
Stmt::Ptr parse_var_decl_stmt(Lexer* lexer)
{
  Var_Decl_Stmt result;
  lexer->expect(ETK_var);

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
  lexer->expect(ETK_Semi);

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
  lexer->expect(ETK_fn);
  result.name = lexer->expect(ETK_Identifier);
  lexer->expect(ETK_LParen);

  std::vector<ParamInfo> temp_arr;

  auto parse_param_push_arr = [lexer,&temp_arr](){
    ParamInfo item;
    item.name = lexer->expect(ETK_Identifier);
    lexer->expect(ETK_Colon);
    item.type = parse_type(lexer);

    temp_arr.emplace_back(std::move(item));
  };

  // is empty params
  if (lexer->peek().type != ETK_RParen)
  {
    parse_param_push_arr();
    while (lexer->peek().type == ETK_Comma)
    {
      lexer->consume();
      parse_param_push_arr();
    }
  }
  lexer->expect(ETK_RParen);

  // Save and Set parsing result
  ParamInfo* params = nullptr;
  if (!temp_arr.empty())
  {
    params = g_arena.alloc<ParamInfo>(temp_arr.size());
    for (int i=0;i<temp_arr.size();++i)
    {
      params[i] = std::move(temp_arr[i]);
    }
  }
  result.type = TypeInfo::Create();
  result.type->tag = ETy_Function;
  result.type->func.params_cnt = temp_arr.size();
  result.type->func.params = params;

  if (lexer->peek().type == ETK_RArrow)
  {
    lexer->consume();
    result.type->func.ret_type = parse_type(lexer);
  }
  else
  {
    result.type->func.ret_type = parse_type(lexer);
  }




  result.body = parse_block_stmt(lexer);

  return ARENA_NEW(Stmt){std::move(result)};
}


Stmt::Ptr parse_stmt(Lexer* lexer)
{
  Stmt::Ptr result = nullptr;
  switch (lexer->peek().type) {
  case ETK_return:   result = parse_ret_stmt(lexer); break;
  case ETK_LBrace:   result = parse_block_stmt(lexer); break;
  case ETK_if:       result = parse_if_stmt(lexer); break;
  case ETK_while:    result = parse_while_stmt(lexer); break;
  case ETK_break:    result = parse_break_stmt(lexer); break;
  case ETK_continue: result = parse_continue_stmt(lexer); break;
  case ETK_var:      result = parse_var_decl_stmt(lexer); break;
  case ETK_fn:       result = parse_fn_decl_stmt(lexer); break;
  default:           result = parse_expr_stmt(lexer); break;
  }

  // HAI_ASSERT(false && "parse_stmt unreachable");
  if (lexer->isPanic)
  {
    result = ARENA_NEW(Stmt){};
    lexer->sync(ETK_Error);
  }


  return result;
}
