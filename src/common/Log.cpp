#include "Log.h"
#include <spdlog/sinks/stdout_color_sinks.h>



// 初始化为 nullptr
std::shared_ptr<spdlog::logger> Log::s_CoreLogger;
std::shared_ptr<spdlog::logger> Log::s_ClientLogger;

static constexpr const char* LOG_DEFAULT_PATTERN = "%^[%T] [%n] [%l] [thread %t] %v%$";


void Log::Init(const char* pattern /*= nullptr*/)
{
  /*
  * flush_on 用于设置 当日志达到指定严重级别时自动刷新到输出目标（如文件），避免因缓冲导致的重要日志丢失。
  * set_level 设置logger日志输出最低等级
  * flush_every  定时刷新一次
  */


  /// Global settings

  spdlog::set_pattern(pattern ? pattern : LOG_DEFAULT_PATTERN);
  //spdlog::flush_every(std::chrono::seconds(1)); // flush to file faster
  //spdlog::flush_on(spdlog::level::err);


  /// Custom

  s_CoreLogger = spdlog::stdout_color_mt("CORE");

  s_CoreLogger->set_level(spdlog::level::trace);


  s_ClientLogger = spdlog::stdout_color_mt("APP");
  s_ClientLogger->set_level(spdlog::level::trace);


  /// for spdlog::get("logger_name");
  // FIXME: throw_if_exists_ here ？
  /*spdlog::register_logger(s_CoreLogger);
  spdlog::register_logger(s_ClientLogger);*/
}

void Log::Shutdown()
{
  s_ClientLogger.reset();
  s_CoreLogger.reset();
  spdlog::drop_all();
}

