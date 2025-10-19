#pragma once

#include "spargel/base/check.h"
#include "spargel/base/span.h"
#include "spargel/base/types.h"

namespace spargel::base {

    /// A wrapper of the builtin array type `T[N]`.
    ///
    template <typename T, usize size>
    struct InlineArray {
        T& operator[](usize i) {
            spargel_check(i < size);
            return _array[i];
        }
        T const& operator[](usize i) const {
            spargel_check(i < size);
            return _array[i];
        }

        usize count() const { return size; }

        T* begin() { return _array; }
        T const* begin() const { return _array; }
        T* end() { return _array + size; }
        T const* end() const { return _array + size; }

        Span<T> asSpan() const { return Span<T>{_array, _array + size}; }
        Span<T> span() const { return Span<T>{_array, _array + size}; }

        T _array[size];
    };

}  // namespace spargel::base
