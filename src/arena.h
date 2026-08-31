#ifndef HAILANG_ARENA_H
#define HAILANG_ARENA_H
#pragma once

#include <cstdint>

#include "common/Assert.h"

// TODO: Arena
#ifndef NDEBUG

#define G_WATCHER(name) inline size_t g_##name##_allocted_size = 0

#define G_OPERATOR_NEW_WATCHER_BY(name) \
void* operator new(size_t size){\
  g_##name##_allocted_size += size;\
  return ::operator new(size);\
}\
void operator delete(void* ptr,size_t size) {\
  g_##name##_allocted_size -= size;\
  return ::operator delete(ptr);\
}

#define G_WATCHER_AOP(name,code) do {\
  LOG_INFO("total alloc {} bytes",g_##name##_allocted_size);\
  {code}\
  if( g_##name##_allocted_size != 0){\
    LOG_INFO("allocator leak {} bytes",g_##name##_allocted_size);\
  }\
}while (0)


#endif

struct Arena
{
  size_t size_ = 0;
  size_t cap_ = 0;
  uint8_t* pool_ = nullptr;

  bool init(uint8_t* resource,size_t cap)
  {
    pool_ = resource;
    cap_ = cap;
    return true;
  }
  size_t save_point() const
  {
    return size_;
  }
  void rewind(const size_t save_p)
  {
    size_ = save_p;
  }

  void* alloc(const size_t size)
  {
    HAI_ASSERT(size != 0 && "Arena can't alloc 0 ");
    HAI_ASSERT(size_ + size <= cap_ && "Arena empty resource");

    void* ptr = pool_ + size_;
    size_ += size;
    return ptr;
  }

  template<typename T>
  T* alloc_as(const size_t cnt = 1)
  {
    return static_cast<T*>(this->alloc(sizeof(T) * cnt));
  }

  // only call when the pool is on heap(by new)
  void free()
  {
    delete pool_;
  }
};



#endif //HAILANG_ARENA_H
