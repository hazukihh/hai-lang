#include "syntax_parser.h"

#include "lexer.h"

void AstNode::print(std::string (*get_value)(const Token& t), int depth) const
{

}


/// ==============
/// Internal Decl
/// ==============


/**
 * expr_stmt ::= expr? ";"
 */
static AstNode::Ptr parse_expr_stmt(Lexer *lexer);

/**
 * ret_stmt ::= "return" expr? ";"
 */
static AstNode::Ptr parse_ret_stmt(Lexer *lexer);

/**
 * block_stmt ::= "{" stmt* "}"
 */
static AstNode::Ptr parse_block_stmt(Lexer *lexer);

/**
 * if_stmt ::= "if" expr block_stmt ["else" block_stmt | "else" if_stmt]
 */
static AstNode::Ptr parse_if_stmt(Lexer *lexer);


/// =======
///  Impl
/// =======


AstNode::Ptr parse_expr_stmt(Lexer* lexer)
{
  assert(false && "TODO impl parse_expr_stmt");
  return nullptr;

  return nullptr;
  // if (lexer->peek().type == ETK_Semi)
  // {
  //   lexer->consume();
  //   return std::make_unique<AstNode>(AstNode{
  //     .kind = ENodeKind::ND_NULL_STMT
  //   });
  // }
  //
  // auto left = parse_expression(lexer,0);
  // lexer->skip(ETK_Semi);
  //
  //
  // return std::make_unique<AstNode>(AstNode{
  //   ENodeKind::ND_EXPR_STMT,
  //   std::move(*left.release())
  // });

}

AstNode::Ptr parse_ret_stmt(Lexer* lexer)
{
  assert(false && "TODO impl parse_ret_stmt");
  return nullptr;

  // lexer->skip(ETK_return);
  // return std::make_unique<AstNode>(AstNode{
  //   .kind = ENodeKind::ND_RETURN_STMT,
  //
  // });
}

AstNode::Ptr parse_block_stmt(Lexer* lexer)
{
  assert(false && "TODO impl parse_block_stmt");
  return nullptr;

  // lexer->skip(ETK_LBrace);
  //
  // AstNode head;
  // AstNode* p = &head;
  // while (lexer->peek().type != ETK_RBrace)
  // {
  //   p->next = parse_stmt(lexer);
  //   p = p->next.get();
  // }
  // lexer->skip(ETK_RBrace);
  //
  // return std::make_unique<AstNode>(AstNode{
  //   .left = std::move(head.next),
  //   .kind = ENodeKind::ND_BLOCK_STMT,
  // });
}

AstNode::Ptr parse_if_stmt(Lexer* lexer)
{
  lexer->skip(ETK_if);
  auto p1 = parse_expression(lexer);
  auto p2 = parse_block_stmt(lexer);
  if (lexer->peek().type == ETK_else)
  {
    lexer->consume();
    if (lexer->peek().type == ETK_LBrace)
    {
      auto p3 = parse_block_stmt(lexer);
    }
    else
    {
      auto p3 = parse_if_stmt(lexer);
    }
  }
  assert(false && "TODO impl, p1 p2 p3");
  return nullptr;
}


AstNode::Ptr parse_stmt(Lexer* lexer)
{
  switch (lexer->peek().type) {
  case ETK_return: return parse_ret_stmt(lexer);
  case ETK_LBrace: return parse_block_stmt(lexer);
  case ETK_if:     return parse_if_stmt(lexer);
    // TODO: var_decl, fn_decl
  //case ETK_var:    return parse_var_decl_stmt(lexer);
  //case ETK_fn:     return parse_fn_decl_stmt(lexer);
  default:         return parse_expr_stmt(lexer);
  }

  assert(false && "parse_stmt unreachable");
}
