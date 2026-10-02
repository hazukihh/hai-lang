#ifndef HAILANG_SYMBOL_TABLE_H
#define HAILANG_SYMBOL_TABLE_H
#pragma once

#include <parallel_hashmap/phmap.h>

#include "common/StringView.h"
#include "arena.h"

// pmr::string vs arena
struct StringPool
{

  Arena str_arena;
  phmap::flat_hash_set<StringView> stringSet;

  static StringPool& GetInstance();


  StringView CreateString(StringView sv)
  {
    auto it = stringSet.find(sv);
    if(it != stringSet.end()) {
      return *it;
    }

    char* buf = str_arena.strdup(sv);

    auto [new_it, inserted] = stringSet.emplace(StringView(buf, sv.size()));
    return *new_it;
  }
};


inline StringPool& StringPool::GetInstance() {
  static StringPool pool;
  return pool;
}




#endif //HAILANG_SYMBOL_TABLE_H
