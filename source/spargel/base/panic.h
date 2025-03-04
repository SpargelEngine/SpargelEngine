#pragma once

#include <spargel/base/types.h>

namespace spargel::base {

    // [[noreturn]] void panic();

    [[noreturn]] void panic_at(char const* msg, char const* file, char const* func, u32 line);

#define spargel_panic_here() ::spargel::base::panic_at("<unknown>", __FILE__, __func__, __LINE__)
#define spargel_panic(msg) ::spargel::base::panic_at(msg, __FILE__, __func__, __LINE__)

}  // namespace spargel::base
