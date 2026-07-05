#ifndef _ENGINE_STRING_VIEW_HPP
#define _ENGINE_STRING_VIEW_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)


#include <string>
#include <cstring> // memcmp
using StringView = ::std::string_view;


[[nodiscard]] ::std::string sv_to_str(StringView sv);
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


#ifdef STRING_VIEW_IMPLEMENTATION

#include <cctype> // for std::isspace


::std::string sv_to_str(StringView sv) {
  return ::std::string(sv.begin(), sv.end());
}

StringView sv_trim_left(StringView sv) {
  size_t i = 0;
  size_t size = sv.size();
  const char* data = sv.data();
  while (i < size && isspace(data[i])) {
    i += 1;
  }
  return StringView(data + i, size - i);
}
StringView sv_trim_right(StringView sv) {
  size_t i = 0;
  size_t size = sv.size();
  const char* data = sv.data();
  while (i < size && isspace(data[size - 1 - i])) {
    i += 1;
  }

  return StringView(data, size - i);
}
StringView sv_trim(StringView sv) {
  return sv_trim_right(sv_trim_left(sv));
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
    return StringView();
  }
  if (end > sv.size()) {
    end = sv.size();
  }
  if (start >= end) {
    return StringView();
  }

  return StringView(sv.data() + start, end - start);
}

StringView sv_slice(StringView sv, size_t start) {
  return sv_slice(sv, start, sv.size());
}

#endif // STRING_VIEW_IMPLEMENTATION

#endif // !_STRING_UTILS_HPP_
