#ifndef HAILANG_NUMBERS_H
#define HAILANG_NUMBERS_H
#include <charconv>
#include "StringView.h"


// TODO: excepted<Number,bool>
template<typename Number>
Number parse_number(StringView text)
{
  Number val;
  auto result = std::from_chars(
        text.data(),text.data()+text.size(),
        val
        );
  HAI_ASSERT(result.ec == std::errc{}
    && result.ptr == text.data()+text.size()
    && "parse NumberLit failed");
  return val;
}

#endif //HAILANG_NUMBERS_H
