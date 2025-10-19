#pragma once

namespace spargel::core {
    template <typename T>
    using remove_cvref = __remove_cvref(T);
}
