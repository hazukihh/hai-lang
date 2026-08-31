#ifndef HAILANG_TYPE_H
#define HAILANG_TYPE_H
#pragma once

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

inline uint8_t g_buffer_typeinfo[1024*4];
inline Arena g_arena_typeinfo {
  .size_ = 0,
  .cap_ = sizeof(g_buffer_typeinfo),
  .pool_ = g_buffer_typeinfo
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

    Token tok = Token{};
  };

  std::string to_str() const
  {
    fmt::memory_buffer buf;
    const TypeInfo* p = this;
    while (p) {
      switch (p->tag)
      {
      case ETy_Builtin:
        buf.append(p->tok.text);
        p = nullptr;
        break;
      case ETy_Pointer:
        buf.push_back('*');
        p = p->pointer.base;
        break;
      case ETy_Array:
        if (p->arr.size == 0) {
          buf.append(sv_from_lit("[]"));
        } else {
          fmt::format_to(std::back_inserter(buf),"[{}]",p->arr.size);
        }
        p = p->arr.base;
        break;
      case ETy_Function:
        buf.append(sv_from_lit("fn_type_TODO"));p = nullptr;
        break;
      case ETy_Named:
        buf.append(p->tok.text);
        p = nullptr;
        break;
      default:
        HAI_ASSERT(false && "unreachable");
        buf.append(sv_from_lit("<Invalid_Type>"));
        p=nullptr;
        break;
      }
    }
    return fmt::to_string(buf);
  }

  static TypeInfo* Create()
  {
    return g_arena_typeinfo.alloc_as<TypeInfo>();
  }
};



// TODO: move to symbol_table.h??
struct IdentInfo
{
  Token name;
  // const Type* type; // defer in sema parsing
};



#endif //HAILANG_TYPE_H
