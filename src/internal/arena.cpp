#include "common/Assert.h"
namespace details {

// #define ARENA_INLINE inline
#define ARENA_ASSERT HAI_ASSERT

#if defined(_WIN32)
#  define ARENA_BACKEND ARENA_BACKEND_WIN32_VIRTUALALLOC
#elif defined(__linux__)
#  define ARENA_BACKEND ARENA_BACKEND_LINUX_MMAP
#endif

#define ARENA_IMPLEMENTATION
#include "tsoding-arena.h"

}
