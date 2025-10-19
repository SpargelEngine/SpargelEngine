#pragma once

namespace spargel::core {
    template <typename S, typename T>
    inline constexpr bool is_same = __is_same(S, T);
}
