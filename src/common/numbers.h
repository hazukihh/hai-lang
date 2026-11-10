#ifndef HAILANG_NUMBERS_H
#define HAILANG_NUMBERS_H
#include <charconv>
#include "StringView.h"


// TODO: excepted<Number,bool>
template<typename Number>
Number parse_number(StringView text,const int base)
{
  Number val;
  // Tips: because the std::from_chars func **for float types** don't have the param "Base";
  //   And FloatTypes default base 10;
  auto result = std::from_chars(
      text.data(),text.data()+text.size(),
      val,base);

  HAI_ASSERT(result.ec == std::errc{}
    && result.ptr == text.data()+text.size()
    && "parse NumberLit failed");
  return val;
}

template<typename Number>
Number parse_number(StringView text)
{
  Number val;
  // Tips: because the std::from_chars func **for float types** don't have the param "Base";
  //   And FloatTypes default base 10;
  auto result = std::from_chars(
        text.data(),text.data()+text.size(),
        val);

  HAI_ASSERT(result.ec == std::errc{}
    && result.ptr == text.data()+text.size()
    && "parse NumberLit failed");
  return val;
}


/**
 * @return {val,ok}
 */
template<typename Number>
std::pair<Number,bool> parse_number_v1(StringView text,const int base = 10)
{
  Number val;
  auto result = base == 10
  ? std::from_chars(
        text.data(),text.data()+text.size(),
        val)
  : std::from_chars(
        text.data(),text.data()+text.size(),
        val,base);

  if(result.ec != std::errc{} || result.ptr != text.data()+text.size()) {
    return {0,false};
  }

  return {val, true};
}

inline int int_from_hex(char c) {
  HAI_ASSERT(
    '0' <= c && c <= '9' ||
    'a' <= c && c <= 'f' ||
    'A' <= c && c <= 'F'
    );

  if ('0' <= c && c <= '9')
    return c - '0';
  if ('a' <= c && c <= 'f')
    return c - 'a' + 10;
  return c - 'A' + 10;
}

#endif //HAILANG_NUMBERS_H
