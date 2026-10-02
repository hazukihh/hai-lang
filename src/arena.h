/**
 * the C++ Wrapper of tsoding-arena.h;
 *  internal/arena: the custom setting for tsoding-arena
 */
#pragma once

#include "common/Assert.h"
#include "common/StringView.h"

#include "internal/arena.h"

using Arena_Mark = details::Arena_Mark;

struct Arena
{
  details::Arena impl;

  ~Arena();

  void *alloc(size_t size_bytes);

  template <typename T>
  T* alloc(const size_t count)
  {
    return static_cast<T*>(this->alloc(sizeof(T) * count));
  }

  void *realloc( void *oldptr, size_t oldsz, size_t newsz);

  char *strdup(StringView sv);

  void *memdup(const void *data, size_t size);

#ifndef ARENA_NOSTDIO
  char *sprintf(const char *format, ...);
#endif // ARENA_NOSTDIO

  Arena_Mark snapshot();
  void reset();
  void rewind(Arena_Mark m);
  void free();
  void trim();
};






