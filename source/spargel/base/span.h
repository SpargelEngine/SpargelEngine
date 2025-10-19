#pragma once

#include "spargel/base/check.h"
#include "spargel/base/format.h"
#include "spargel/base/initializer_list.h"
#include "spargel/base/types.h"

namespace spargel::base {

    namespace span_ {
        // Readonly.
        template <typename T>
        class Span {
        public:
            constexpr Span() = default;

            explicit constexpr Span(T const* begin, T const* end)
                : _begin{begin}, _end{end} {}
            constexpr Span(initializer_list<T> l)
                : _begin{l.begin()}, _end{l.end()} {}

            constexpr T const& operator[](usize i) const { return _begin[i]; }

            constexpr usize count() const {
                spargel_check(_begin <= _end);
                return static_cast<usize>(_end - _begin);
            }
            constexpr T const* data() const { return _begin; }

            constexpr T const* begin() const { return _begin; }
            constexpr T const* end() const { return _end; }

            constexpr Span<Byte> asBytes() const {
                return Span<Byte>(reinterpret_cast<Byte const*>(_begin),
                                  reinterpret_cast<Byte const*>(_end));
            }

        private:
            T const* _begin = nullptr;
            T const* _end = nullptr;
        };
        template <FormatTarget Target, typename T>
        void tag_invoke(tag<formatArg>, Target& target,
                        detail::FormatString fmt, Span<T> s) {
            target.append("[");
            for (usize i = 0; i < s.count(); i++) {
                formatTo(target, fmt, s[i]);
                if (i < s.count() - 1) {
                    target.append(", ");
                }
            }
            target.append("]");
        }
    }  // namespace span_

    using span_::Span;

    template <typename T>
    using span = Span<T>;

    template <typename T>
    Span<T> make_span(usize count, T const* begin) {
        return Span<T>(begin, begin + count);
    }

    template <typename T, usize N>
    Span<T> make_span(T const (&arr)[N]) {
        return Span<T>(arr, arr + N);
    }

}  // namespace spargel::base
