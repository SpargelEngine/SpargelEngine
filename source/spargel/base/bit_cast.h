#pragma once

#include "spargel/base/compiler.h"

namespace spargel::base {

    // TODO: Both S and T need to be trivially copyable.
    template <typename S, typename T>
        requires(sizeof(S) == sizeof(T))
    constexpr T bit_cast(S const& v) {
        // All of Clang, GCC, MSVC have implemented `__builtin_bit_cast`.
        return __builtin_bit_cast(T, v);
    }

}  // namespace spargel::base
