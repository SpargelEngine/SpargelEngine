#pragma once

#include "spargel/base/compiler.h"
#include "spargel/base/types.h"

namespace spargel::base {

    struct SourceLocation {
#if spargel_has_builtin(__builtin_FILE) &&         \
        spargel_has_builtin(__builtin_FUNCTION) && \
        spargel_has_builtin(__builtin_LINE) ||     \
    SPARGEL_IS_MSVC
        static constexpr SourceLocation current(
            char const* file = __builtin_FILE(),
            char const* func = __builtin_FUNCTION(),
            u32 line = __builtin_LINE()) {
            return {file, func, line};
        }
#else
#error unimplemented
#endif
        char const* file_;
        char const* func_;
        u32 line_;
    };

}  // namespace spargel::base
