#ifndef HAILANG_TYPE_H
#define HAILANG_TYPE_H
#pragma once

#include "lexer.h"
#include "common/numbers.h"

// TODO: Type Sys
// TODO: 如何保存对应信息的位置，以用来报错: LineNumberManager??
enum ETypeTag
{
  ETy_Invalid,
  ETy_Builtin,
  ETy_Pointer,
  ETy_Array,
  ETy_Function,
  ETy_Named
};

inline bool IsKeywordBaseType(ETokenType e)
{
  // TODO: assert({s8 u8 ...  string bool void} is consecutive)
  HAI_ASSERT(ETK_s8 + 1 == ETK_u8);

  return ETK_s8 <= e && e <= ETK_void;
}

inline uint8_t g_buffer_typeinfo[1024*4];
inline Arena g_arena_typeinfo {
  .size_ = 0,
  .cap_ = sizeof(g_buffer_typeinfo),
  .pool_ = g_buffer_typeinfo
};


struct TypeInfo;
struct ParamInfo
{
  Token name {ETK_None};
  TypeInfo *type = nullptr;
};

struct TypeInfo
{
  ETypeTag tag = ETypeTag::ETy_Invalid;
  union {
    struct {
      const TypeInfo *base;
    } pointer;

    struct {
      const TypeInfo *base;
      // ==0 : arr_pointer; >=0 : static_arr
      // how?  [...]T : compile size static_arr
      size_t size;
    } arr;

    struct
    {
      uint32_t params_cnt;
      TypeInfo *ret_type ;
      ParamInfo *params;
    } func;

    Token tok = Token{};
  };

  std::string to_str() const
  {
    fmt::memory_buffer buf;
    const TypeInfo* p = this;
    while (p) {
      switch (p->tag) {
      case ETy_Builtin: {
        buf.append(p->tok.text);
        p = nullptr;
      }break;
      case ETy_Pointer: {
        buf.push_back('*');
        p = p->pointer.base;

      }break;
      case ETy_Array: {
        if (p->arr.size == 0) {
          buf.append(sv_from_lit("[]"));
        } else {
          fmt::format_to(std::back_inserter(buf),"[{}]",p->arr.size);
        }
        p = p->arr.base;
      }break;
      case ETy_Function: {
        buf.append(sv_from_lit("fn("));
        for (int i=0;i<p->func.params_cnt;++i)
        {
          if (i!=0) buf.push_back(',');
          buf.append(p->func.params[i].type->to_str());
        }
        fmt::format_to(std::back_inserter(buf),")-> {}",p->func.ret_type->to_str());
        p = nullptr;
      }break;
      case ETy_Named: {
        buf.append(p->tok.text);
        p = nullptr;
      }break;
      default: {
        HAI_ASSERT(false && "unreachable");
        buf.append(sv_from_lit("<Invalid_Type>"));
        p=nullptr;
      }break;
      }
    }
    return fmt::to_string(buf);
  }

  static TypeInfo* Create()
  {
    return g_arena_typeinfo.alloc_as<TypeInfo>();
  }
};

inline TypeInfo* parse_type(Lexer* lexer)
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
      info->arr.size = parse_number<size_t>(next_tok.text);
      lexer->expect(ETK_RBracket);
    } else {
      HAI_ASSERT(false && "unreachable or TODO ETK_Dot3,builtin dyn-array-type");
      lexer->expect(ETK_RBracket);
    }

    info->arr.base = parse_type(lexer);
  }break;
  case ETK_fn: {
    info->tag = ETy_Function;


    lexer->expect(ETK_LParen);
    /// parse fn params list
    std::vector<ParamInfo> temp_arr;
    auto peek_tok = lexer->peek();
    // is empty params
    if (peek_tok.type != ETK_RParen)
    {
      // Match fn(arg1 : T1,arg2 : T2,...) -> T
      if (peek_tok.type == ETK_Identifier && lexer->peek1().type == ETK_Colon) {

        auto parse_param_push_arr = [lexer,&temp_arr](){
          ParamInfo item;
          item.name = lexer->expect(ETK_Identifier);
          lexer->expect(ETK_Colon);
          item.type = parse_type(lexer);

          temp_arr.emplace_back(std::move(item));
        };

        parse_param_push_arr();
        while (lexer->peek().type == ETK_Comma)
        {
          lexer->consume();
          parse_param_push_arr();
        }
      }
      // Match fn(T1,T2,...) -> T
      else {
        auto parse_param_push_arr_1 = [lexer,&temp_arr](){
          ParamInfo item;
          item.type = parse_type(lexer);
          temp_arr.emplace_back(std::move(item));
        };

        parse_param_push_arr_1();
        while (lexer->peek().type == ETK_Comma)
        {
          lexer->consume();
          parse_param_push_arr_1();
        }
      }
    }
    lexer->expect(ETK_RParen);

    /// Save and Set parsing result
    ParamInfo* params = nullptr;
    if (!temp_arr.empty())
    {
      params = g_arena_typeinfo.alloc_as<ParamInfo>( temp_arr.size());
      for (int i=0;i<temp_arr.size();++i)
      {
        params[i] = std::move(temp_arr[i]);
      }
    }
    info->func .params = params,
    info->func.params_cnt = temp_arr.size();


    lexer->expect(ETK_RArrow);
    info->func.ret_type = parse_type(lexer);

  }break;
  case ETK_LParen: {
    info->tag = ETypeTag::ETy_Invalid;
    HAI_ASSERT(false && "TODO: support parsing tuple-type");
  }break;
  case ETK_Identifier: {
    info->tag = ETypeTag::ETy_Named;
    info->tok = t;
  }break;
  default: {
    if (!IsKeywordBaseType(t.type)) {
      lexer->error_at(t,sv_from_lit("expected Type"));
    }
    info->tag = ETypeTag::ETy_Builtin;
    info->tok = t;
  }break;
  }

  // TODO: temp : Type*
  return info;
}




// TODO: move to symbol_table.h??
struct IdentInfo
{
  Token name;
  // const Type* type; // defer in sema parsing
};



#endif //HAILANG_TYPE_H
