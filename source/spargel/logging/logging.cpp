#include "spargel/logging/logging.h"

#include "spargel/base/assert.h"
#include "spargel/config.h"

// libc
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// platform
#if SPARGEL_IS_POSIX || SPARGEL_IS_EMSCRIPTEN
#include <sys/time.h>
#endif

#if SPARGEL_IS_ANDROID
#include <android/log.h>
#endif

#if SPARGEL_IS_EMSCRIPTEN
#include "spargel/base/platform_emscripten.h"
#endif

#if SPARGEL_IS_WINDOWS
// FIXME: move time utils to platform_XXX.cpp
#include <windows.h>
#endif

namespace spargel::logging {

    static char const* log_names[_LOG_COUNT] = {
        "DEBUG", "INFO", "WARN", "ERROR", "FATAL",
    };

    struct log_timestamp {
        int mon;
        int day;
        int hour;
        int min;
        int sec;
        int usec;
    };

    static void log_get_time(struct log_timestamp* time) {
#if SPARGEL_IS_POSIX || SPARGEL_IS_EMSCRIPTEN
        struct timeval tv;
        gettimeofday(&tv, NULL);
        time_t t = tv.tv_sec;
        struct tm local_time;
        localtime_r(&t, &local_time);
        struct tm* tm_time = &local_time;
        time->mon = tm_time->tm_mon + 1;
        time->day = tm_time->tm_mday;
        time->hour = tm_time->tm_hour;
        time->min = tm_time->tm_min;
        time->sec = tm_time->tm_sec;
        time->usec = tv.tv_usec;
#elif SPARGEL_IS_WINDOWS
        SYSTEMTIME system_time;
        GetSystemTime(&system_time);
        time->mon = system_time.wMonth;
        time->day = system_time.wDay;
        time->hour = system_time.wHour;
        time->min = system_time.wMinute;
        time->sec = system_time.wSecond;
        time->usec = system_time.wMilliseconds * 1000;
#else
#error unimplemented
#endif
    }

    void log(int level, char const* file, char const* func, u32 line,
             char const* format, ...) {
        spargel_assert(level >= 0 && level < _LOG_COUNT);

        va_list ap;

#if !SPARGEL_IS_EMSCRIPTEN

#if SPARGEL_ENABLE_LOG_ANSI_COLOR
        char const* log_prefix;
        switch (level) {
        case 0:
            log_prefix = "\033[36m";
            break;
        case 1:
            log_prefix = "\033[32m";
            break;
        case 2:
            log_prefix = "\033[1;93m";
            break;
        case 3:
        case 4:
            log_prefix = "\033[1;31m";
            break;
        default:
            log_prefix = "\033[0m";
            break;
        }
        fputs(log_prefix, stderr);
#endif  // SPARGEL_ENABLE_LOG_ANSI_COLOR

        char const* name = log_names[level];
        struct log_timestamp time;
        log_get_time(&time);

        // Old log format.
        //
        // fprintf(stderr, "[%02d%02d/%02d%02d%02d.%06d:%s:%s:%s:%u] ",
        // time.mon, time.day, time.hour,
        //         time.min, time.sec, time.usec, name, file, func, line);

        fprintf(stderr, "[%02d%02d/%02d%02d%02d.%06d:%s:%s:%s:%u] ", time.mon,
                time.day, time.hour, time.min, time.sec, time.usec, name, file,
                func, line);

        va_start(ap, format);
        vfprintf(stderr, format, ap);
        va_end(ap);

#if SPARGEL_ENABLE_LOG_ANSI_COLOR
        fputs("\033[0m", stderr);
#endif

        fprintf(stderr, "\n");

#endif  // !SPARGEL_IS_EMSCRIPTEN

#if SPARGEL_IS_ANDROID

        int android_log_prio;
        switch (level) {
        case 0:
            android_log_prio = ANDROID_LOG_DEBUG;
            break;
        case 1:
            android_log_prio = ANDROID_LOG_INFO;
            break;
        case 2:
            android_log_prio = ANDROID_LOG_WARN;
            break;
        case 3:
            android_log_prio = ANDROID_LOG_ERROR;
            break;
        case 4:
            android_log_prio = ANDROID_LOG_FATAL;
            break;
        default:
            android_log_prio = ANDROID_LOG_UNKNOWN;
            break;
        }
        va_start(ap, format);
        __android_log_vprint(android_log_prio, "spargel", format, ap);
        va_end(ap);

#elif SPARGEL_IS_EMSCRIPTEN

        // FIXME
        char log_buf[4096];

        va_start(ap, format);
        vsnprintf(log_buf, 4096, format, ap);
        va_end(ap);

        switch (level) {
        case 0:
            emscriptenConsoleDebug(log_buf);
            break;
        case 1:
            emscriptenConsoleInfo(log_buf);
            break;
        case 2:
            emscriptenConsoleWarn(log_buf);
            break;
        case 3:
        case 4:
            emscriptenConsoleError(log_buf);
            break;
        default:
            emscriptenConsoleLog(log_buf);
            break;
        }

#endif  // SPARGEL_IS_EMSCRIPTEN
    }

}  // namespace spargel::logging
