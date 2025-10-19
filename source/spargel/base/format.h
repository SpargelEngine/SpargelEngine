#pragma once

#include "spargel/base/algorithm.h"
#include "spargel/base/tag_invoke.h"

namespace spargel::base {
    // TODO
    template <typename T>
    concept FormatTarget = true;

    namespace detail {
        // TODO: Type-safe `FormatString`.
        class FormatString {
        public:
            constexpr FormatString() : begin_{nullptr}, end_{nullptr} {}
            constexpr FormatString(char const* s)
                : FormatString{s, s + __builtin_strlen(s)} {}
            constexpr FormatString(char const* begin, char const* end)
                : begin_{begin}, end_{end} {}

            constexpr char const* begin() const { return begin_; }
            constexpr char const* end() const { return end_; }

            constexpr bool empty() const { return begin_ == nullptr; }

            constexpr void split(FormatString& segment, FormatString& fmt,
                                 FormatString& rest) const {
                auto iter = begin_;
                while (iter < end_) {
                    if (*iter == '}') {
                        auto next = iter + 1;
                        if (next >= end_) {
                            __builtin_trap();
                        }
                        if (*next != '}') {
                            __builtin_trap();
                        }
                        iter += 2;
                        continue;
                    }
                    if (*iter != '{') {
                        iter++;
                        continue;
                    }
                    // *iter == '{'
                    if (iter + 1 >= end_) {
                        // bad case: "xxx{"
                        segment = FormatString{begin_, end_};
                        fmt = FormatString{};
                        rest = FormatString{};
                    }
                    auto next = *(iter + 1);
                    if (next == '{') {
                        // escape case: "xxx{{yyy"
                        iter += 2;
                        continue;
                    }
                    // fmt case: "xxx{yyy"
                    segment = FormatString{begin_, iter};
                    auto fmt_start = iter;
                    while (iter < end_) {
                        if (*iter == '}') {
                            break;
                        }
                        iter++;
                    }
                    // *iter == '}' or iter == end_
                    if (iter >= end_) {
                        __builtin_trap();
                    }
                    // *iter == '}', iter < end_
                    iter++;
                    fmt = FormatString{fmt_start, iter};
                    rest = FormatString{iter, end_};
                    return;
                }
                if (iter == end_) {
                    segment = FormatString{begin_, end_};
                    fmt = FormatString{};
                    rest = FormatString{};
                }
            }

        private:
            char const* begin_;
            char const* end_;
        };
    }  // namespace detail

    namespace format_ {
        struct FormatArgCPO {
            template <FormatTarget Target, typename Arg>
            constexpr decltype(auto) operator()(Target& target,
                                                detail::FormatString fmt,
                                                Arg&& arg) const {
                return tag_invoke(FormatArgCPO{}, target, fmt,
                                  base::forward<Arg>(arg));
            }
        };
    }  // namespace format_

    inline constexpr format_::FormatArgCPO formatArg;

    namespace format_ {
        template <FormatTarget Target>
        void tag_invoke(tag<formatArg>, Target& target, detail::FormatString,
                        char const* s) {
            target.append(s, s + __builtin_strlen(s));
        }
        template <FormatTarget Target>
        void formatInteger(Target& target, usize n) {
            if (n == 0) {
                target.append("0");
                return;
            }
            char buf[21];
            int i = 0;
            while (n > 0) {
                buf[i++] = '0' + n % 10;
                n /= 10;
            }
            buf[i] = '\0';
            for (int j = 0; j <= i / 2; j++) {
                swap(buf[j], buf[i - j]);
            }
            target.append(buf, buf + i + 1);
        }
        template <FormatTarget Target>
        void tag_invoke(tag<formatArg>, Target& target, detail::FormatString,
                        usize n) {
            formatInteger(target, n);
        }
    }  // namespace format_

    template <FormatTarget Target>
    constexpr void formatTo(Target& target, detail::FormatString fmt) {
        target.append(fmt.begin(), fmt.end());
    }

    template <FormatTarget Target, typename Arg, typename... Args>
    constexpr void formatTo(Target& target, detail::FormatString fmt, Arg&& arg,
                            Args&&... args) {
        detail::FormatString segment;
        detail::FormatString fmt_str;
        detail::FormatString rest;
        fmt.split(segment, fmt_str, rest);
        target.append(segment.begin(), segment.end());
        formatArg(target, fmt_str, arg);
        formatTo(target, rest, forward<Args>(args)...);
    }

    namespace detail {
        struct ConsoleTarget {
            void append(char const* begin, char const* end);
            void append(char const* s) { append(s, s + __builtin_strlen(s)); }
        };
    }  // namespace detail

    template <typename... Args>
    void print(detail::FormatString fmt, Args&&... args) {
        detail::ConsoleTarget target;
        formatTo(target, fmt, base::forward<Args>(args)...);
    }
}  // namespace spargel::base
