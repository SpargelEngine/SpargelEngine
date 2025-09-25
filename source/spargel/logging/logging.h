#pragma once

#include "spargel/base/compiler.h"
#include "spargel/base/format.h"
#include "spargel/base/meta.h"
#include "spargel/base/source_location.h"
#include "spargel/base/types.h"

#if SPARGEL_IS_CLANG || SPARGEL_IS_GCC
#define FILE_NAME_ __FILE_NAME__
#else
#define FILE_NAME_ __FILE__
#endif

#define LOG_IMPL(level, ...) \
    ::spargel::logging::log(level, FILE_NAME_, __func__, __LINE__, __VA_ARGS__)
#define spargel_log_debug(...) \
    LOG_IMPL(::spargel::logging::LOG_DEBUG, __VA_ARGS__)
#define spargel_log_info(...) \
    LOG_IMPL(::spargel::logging::LOG_INFO, __VA_ARGS__)
#define spargel_log_warn(...) \
    LOG_IMPL(::spargel::logging::LOG_WARN, __VA_ARGS__)
#define spargel_log_error(...) \
    LOG_IMPL(::spargel::logging::LOG_ERROR, __VA_ARGS__)
#define spargel_log_fatal(...) \
    LOG_IMPL(::spargel::logging::LOG_FATAL, __VA_ARGS__);

namespace spargel::logging {

    enum log_level {
        /* message for debugging */
        LOG_DEBUG = 0,
        /* general log events */
        LOG_INFO,
        /* warning, not necessarily shown to users */
        LOG_WARN,
        /* error that can recover from */
        LOG_ERROR,
        /* nothing more can be done other than aborting */
        LOG_FATAL,

        _LOG_COUNT,
    };

    SPARGEL_ATTRIBUTE_PRINTF_FORMAT(5, 6)
    void log(int level, char const* file, char const* func, u32 line,
             char const* format, ...);

    namespace detail {
        class LogFormat {
        public:
            constexpr LogFormat(
                char const* s,
                base::source_location loc = base::source_location::current())
                : fmt_{s}, loc_{loc} {}

            constexpr base::detail::FormatString format() const { return fmt_; }
            constexpr base::source_location location() const { return loc_; }

        private:
            base::detail::FormatString fmt_;
            base::source_location loc_;
        };
    }  // namespace detail

    template <typename... Args>
    void info(detail::LogFormat fmt, Args&&... args) {
        base::print("[INFO:{}] ", fmt.location()._file);
        base::print(fmt.format(), base::forward<Args>(args)...);
        base::print("\n");
    }

}  // namespace spargel::logging
