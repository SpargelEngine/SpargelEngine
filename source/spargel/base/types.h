#pragma once

#include "spargel/base/compiler.h"
#include "spargel/config.h"

// c std
#include <stddef.h>
#include <stdint.h>

// cpp std
// #include <cstddef>  // std::byte

using ssize = ptrdiff_t;
using usize = size_t;

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using nullptr_t = decltype(nullptr);

// TODO: not well supported by MSVC or older GCC/Clang
// #if !(SPARGEL_IS_MSVC)
// using f16 = _Float16;
// #endif

using f32 = float;
using f64 = double;

// namespace std {
//     enum class byte : unsigned char {};
// }

namespace spargel::base {

    // using Byte = std::byte;
    using Byte = u8;

    using nullptr_t = decltype(nullptr);

}  // namespace spargel::base
