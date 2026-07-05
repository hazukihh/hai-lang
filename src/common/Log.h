#ifndef _LOG_H
#define _LOG_H
#pragma once
#include <memory>
#include <spdlog/spdlog.h>



class Log
{
private:
  static std::shared_ptr<spdlog::logger> s_CoreLogger;
  static std::shared_ptr<spdlog::logger> s_ClientLogger;

public:
  /**
  * @param[in] pattern
  *   %^ ... %$  : Color Range
  *   [%T]		: HH:MM:SS格式的当前时间
  *   %n		:  Logger的名字
  *   %l    : level
  *     Trace < Debug < Info <
  *       Warn < Error < Critical < Off(Disables all logging)
  *   %t    : 线程id
  *   %v		: 实际的文字内容
  */
  static void Init(const char* pattern = nullptr);
  static void Shutdown();

  inline static std::shared_ptr<spdlog::logger>& GetCoreLogger()
  {
    return s_CoreLogger;
  }

  inline static std::shared_ptr<spdlog::logger>& GetClientLogger()
  {
    return s_ClientLogger;
  }


};



// Log Macros

#define LOG_CORE_TRACE(...)	::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define LOG_CORE_INFO(...)	::Log::GetCoreLogger()->info(__VA_ARGS__)
#define LOG_CORE_WARN(...)	::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define LOG_CORE_ERROR(...)	::Log::GetCoreLogger()->error(__VA_ARGS__)
//#define LOG_CORE_FATAL(...)	::Log::GetCoreLogger()->fatal(__VA_ARGS__)
#define LOG_CORE_FATAL(...)	::Log::GetCoreLogger()->critical(__VA_ARGS__)

#define LOG_TRACE(...)	::Log::GetClientLogger()->trace(__VA_ARGS__)
#define LOG_INFO(...)	::Log::GetClientLogger()->info(__VA_ARGS__)
#define LOG_WARN(...)	::Log::GetClientLogger()->warn(__VA_ARGS__)
#define LOG_ERROR(...)	::Log::GetClientLogger()->error(__VA_ARGS__)
//#define LOG_FATAL(...)	::Log::GetClientLogger()->fatal(__VA_ARGS__)
#define LOG_FATAL(...)	::Log::GetClientLogger()->critical(__VA_ARGS__)

#if DEBUG || _DEBUG

#define LOG_CORE_DEBUG(...)	::Log::GetCoreLogger()->debug(__VA_ARGS__)

#define LOG_DEBUG(...)	::Log::GetClientLogger()->debug(__VA_ARGS__)

#else
#define LOG_CORE_DEBUG(...)
#define LOG_DEBUG(...)
#endif



#endif // !_LOG_H
