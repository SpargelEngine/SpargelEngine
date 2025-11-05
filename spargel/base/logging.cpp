#include "spargel/base/logging.h"

#include <stdarg.h>
#include <stdio.h>

#include "spargel/base/unused.h"

namespace spargel::base {
namespace {

char const* level_name(log_level level) {
  switch (level) {
  case log_level::debug:
    return "DEBUG";
  case log_level::info:
    return "INFO";
  case log_level::warning:
    return "WARNING";
  case log_level::error:
    return "ERROR";
  case log_level::fatal:
    return "FATAL";
  }
}

}  // namespace

void log_impl(log_level level, char const* file, char const* func,
              unsigned int line, char const* fmt, ...) {
  UNUSED(file);
  UNUSED(func);
  UNUSED(line);

  va_list ap;

  fprintf(stderr, "[%s] ", level_name(level));

  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);

  fprintf(stderr, "\n");
}

}  // namespace spargel::base
