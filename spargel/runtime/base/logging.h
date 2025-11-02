#pragma once

#define LOG_IMPL_(level, ...)                                            \
  ::spargel::base::log_impl(::spargel::base::log_level::level, __FILE__, \
                            __func__, __LINE__, __VA_ARGS__)
#define LOG_DEBUG(...) LOG_IMPL_(debug, __VA_ARGS__)
#define LOG_INFO(...) LOG_IMPL_(info, __VA_ARGS__)
#define LOG_WARNING(...) LOG_IMPL_(warning, __VA_ARGS__)
#define LOG_ERROR(...) LOG_IMPL_(error, __VA_ARGS__)
#define LOG_FATAL(...) LOG_IMPL_(fatal, __VA_ARGS__)

namespace spargel::base {

enum class log_level {
  debug,
  info,
  warning,
  error,
  fatal,
};

void log_impl(log_level level, char const* file, char const* func,
              unsigned int line, char const* msg, ...)
    __attribute__((__format__(__printf__, 5, 6)));

}  // namespace spargel::base
