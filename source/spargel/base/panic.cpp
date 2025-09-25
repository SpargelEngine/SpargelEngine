#include "spargel/base/panic.h"

#include "spargel/base/backtrace.h"
#include "spargel/base/compiler.h"

/* libc */
#include <stdio.h>

namespace spargel::base {

    /* there is no need to report allocation or check memory leaks if the
     * program panics */
    void panic_at(char const* msg, char const* file, char const* func,
                  u32 line) {
        /* todo: rewrite fprintf */
        fprintf(stderr, "======== PANIC [%s:%s:%u] ========\n", file, func,
                line);
        fprintf(stderr, "%s\n", msg);
        PrintBacktrace();
#if defined(SPARGEL_IS_CLANG) || defined(SPARGEL_IS_GCC)
        __builtin_trap();
#elif defined(SPARGEL_IS_MSVC)
        __assume(false);
#else
#error unimplemented
#endif
    }

}  // namespace spargel::base
