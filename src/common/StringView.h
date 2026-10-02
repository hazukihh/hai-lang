#ifndef _ENGINE_STRING_VIEW_HPP
#define _ENGINE_STRING_VIEW_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)


#include <string>
#include <cstring> // memcmp
using StringView = ::std::string_view;

// FIX: when use isspace{' ','\t',\n','\v','\f','\r'}, if c not int -1~0~255 ,(utf-8) will error
inline bool is_space(int c)
{
  if (c == -1) return false;
  return isspace(static_cast<uint8_t>(c));
}
inline bool is_digit(int c)
{
  if (c == -1) return false;
  return isdigit(static_cast<uint8_t>(c));
}
inline bool is_alpha(int c)
{
  if (c == -1) return false;
  return isalpha(static_cast<uint8_t>(c));
}
inline bool is_alnum(int c)
{
  if (c == -1) return false;
  return isalnum(static_cast<uint8_t>(c));
}


template<size_t N>
[[nodiscard]] constexpr StringView sv_from_lit(const char (&cstr)[N])
{
  return StringView{cstr, N - 1};
}

[[nodiscard]] ::std::string sv_to_str(StringView sv);

[[nodiscard]] StringView sv_trim_left_if(StringView sv,bool (*pre)(int));
[[nodiscard]] StringView sv_trim_right_if(StringView sv,bool (*pre)(int));
[[nodiscard]] StringView sv_trim_if(StringView sv,bool (*pre)(int));
/**
 * @brief trim_left the char in {' ','\t',\n','\v','\f','\r'}(isspace)
 */
[[nodiscard]] StringView sv_trim_left(StringView sv);
[[nodiscard]] StringView sv_trim_right(StringView sv);
[[nodiscard]] StringView sv_trim(StringView sv);

[[nodiscard]] bool sv_eq(StringView a, StringView b);
[[nodiscard]] bool sv_ends_with(StringView sv, StringView suffix);
[[nodiscard]] bool sv_starts_with(StringView sv, StringView prefix);
[[nodiscard]] bool sv_divide(StringView sv, int delim, StringView& left, StringView& right);
[[nodiscard]] // return sv[start,end)
[[nodiscard]] StringView sv_slice(StringView sv, size_t start, size_t end);
[[nodiscard]] // return sv[statr,sv.size())
[[nodiscard]] StringView sv_slice(StringView sv, size_t start);
template<typename... Args>
void sv_cat_to(std::string& buffer,StringView sv1,StringView sv2,Args&&... args);

template<typename... Args>
[[nodiscard]] std::string sv_cat(StringView sv1,StringView sv2,Args&&... args);

#ifdef STRING_VIEW_IMPLEMENTATION

// template <size_t N>
// constexpr StringView sv_from_lit(const char(& cstr)[N])
// {
//   return StringView{cstr, N - 1};
// }

::std::string sv_to_str(StringView sv) {
  return ::std::string(sv.begin(), sv.end());
  // c++17 20:
  // return ::std::string(sv);
}


StringView sv_trim_left_if(StringView sv,bool (*pre)(int)) {
  size_t i = 0;
  size_t size = sv.size();
  const char* data = sv.data();
  while (i < size && pre(data[i])) {
    i += 1;
  }
  return StringView{data + i, size - i};
}
StringView sv_trim_right_if(StringView sv,bool (*pre)(int)) {
  size_t i = 0;
  size_t size = sv.size();
  const char* data = sv.data();
  while (i < size && pre(data[size - 1 - i])) {
    i += 1;
  }

  return StringView{data, size - i};

}
StringView sv_trim_if(StringView sv,bool (*pre)(int)) {
  return sv_trim_right_if(sv_trim_left_if(sv,pre),pre);
}

StringView sv_trim_left(StringView sv) {
  return sv_trim_left_if(sv,is_space);
}
StringView sv_trim_right(StringView sv) {
  return sv_trim_right_if(sv,is_space);
}
StringView sv_trim(StringView sv) {
  return sv_trim_if(sv,is_space);
}



bool sv_eq(StringView a, StringView b) {
  if (a.size() != b.size()) {
    return false;
  }
  else {
    return memcmp(a.data(), b.data(), a.size()) == 0;
  }
}

bool sv_ends_with(StringView sv, StringView suffix) {
  if (sv.size() >= suffix.size()) {
    StringView sv_tail(
      sv.data() + sv.size() - suffix.size(),
      suffix.size()
    );
    return sv_eq(sv_tail, suffix);
  }
  return false;
}

bool sv_starts_with(StringView sv, StringView prefix) {
  if (prefix.size() <= sv.size()) {
    StringView actual_prefix(sv.data(), prefix.size());
    return sv_eq(prefix, actual_prefix);
  }
  return false;
}

/**
 * @param left,right
 *   if the delimiter is not found, sv_right is empty.
 *   if the delimiter in the first pos, sv_left is empty
 *   if the delimiter in the last pos, sv_right is empty.
 * @return the delim is be found or not;
 */
bool sv_divide(StringView sv, int delim, StringView& left, StringView& right) {
  size_t size = sv.size();
  const char* data = sv.data();
  size_t i = 0;
  while (i < size && data[i] != delim) {
    i += 1;
  }

  left = StringView(data, i);

  if (i < size) {
    right = StringView(data + i + 1, size - (i + 1));
    return true;
  }
  else {
    right = StringView(data + i, size - i);
    return false;
  }

}


StringView sv_slice(StringView sv, size_t start, size_t end) {
  if (start >= sv.size()) {
    return StringView{sv.data() + sv.size(),0};
  }
  if (end > sv.size()) {
    end = sv.size();
  }
  if (start >= end) {
    return StringView{sv.data() + end,0};
  }

  return StringView{sv.data() + start, end - start};
}

StringView sv_slice(StringView sv, size_t start) {
  return sv_slice(sv, start, sv.size());
}

template<typename... Args>
void sv_cat_to(std::string& buffer,StringView sv1,StringView sv2,Args&&... args) {

  constexpr size_t arr_size = sizeof...(Args) + 2;
  if constexpr(arr_size == 2) {
    buffer.clear();
    buffer.append(sv1.data(), sv1.size());
    buffer.append(sv2.data(), sv2.size());
    return;
  }
  StringView sv_arr[] = {sv1,sv2,args...};

  size_t new_size = 0;
  for(size_t i = 0; i < arr_size; ++i) {
    new_size += sv_arr[i].size();
  }

  buffer.resize(new_size);
  size_t pos = 0;
  for(size_t i = 0; i < arr_size; ++i) {
    memcpy(&buffer[pos], sv_arr[i].data(), sv_arr[i].size());
    pos += sv_arr[i].size();
  }
}

template<typename... Args>
std::string sv_cat(StringView sv1,StringView sv2,Args&&... args) {
  std::string result;
  sv_cat_to(result,sv1,sv2,args...);
  return result;
}

#endif // STRING_VIEW_IMPLEMENTATION

#endif // !_STRING_UTILS_HPP_
