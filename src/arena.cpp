#include "arena.h"



Arena::~Arena()
{
  this->free();
}

void* Arena::alloc(size_t size_bytes)
{
  return details::arena_alloc(&impl,size_bytes);
}



void* Arena::realloc(void* oldptr, size_t oldsz, size_t newsz)
{
  return details::arena_realloc(&impl,oldptr,oldsz,newsz);
}

char* Arena::strdup(StringView sv)
{
  size_t n = sv.size();
  char *dup = static_cast<char*>(this->alloc(n + 1));
  std::memcpy(dup, sv.data(), n);
  dup[n] = '\0';
  return dup;
}


void* Arena::memdup(const void* data, size_t size)
{
  void *dest = this->alloc(size);
  std::memcpy(dest, data, size);
  return dest;
}

char* Arena::sprintf(const char* format, ...)
{
  va_list args;
  va_start(args, format);
  char *result = details::arena_vsprintf(&impl, format, args);
  va_end(args);

  return result;
}

Arena_Mark Arena::snapshot()
{
  return details::arena_snapshot(&impl);
}

void Arena::reset()
{
  details::arena_reset(&impl);
}

void Arena::rewind(Arena_Mark m)
{
  details::arena_rewind(&impl,m);
}

void Arena::free()
{
  details::arena_free(&impl);
}

void Arena::trim()
{
  details::arena_trim(&impl);
}

