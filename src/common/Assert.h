#ifndef _ASSERT_H
#define _ASSERT_H

#include <spdlog/fmt/bundled/compile.h>
#include <debugbreak.h>
#include "Log.h"



#ifndef NDEBUG
# define HAI_ASSERT(expr) do{\
    if (!(expr)) [[unlikely]]{\
      LOG_ERROR(SPDLOG_FMT_STRING("Assertion failed at {}:{}, {}"),__FILE__,__LINE__,#expr);\
      debug_break();\
    }\
  }while (0)


# define HAI_UNREACHABLE() HAI_ASSERT(false && "UnReachable")
#else
# define HAI_ASSERT(expr) ((void)0)
# define HAI_UNREACHABLE
#endif


#endif //_ASSERT_H
