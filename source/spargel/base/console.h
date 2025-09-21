#pragma once

#include "spargel/base/string_view.h"

namespace spargel::base {
    // Buffered output to stdout.
    //
    // Warning: The buffer is not flushed when panic. The implementation can
    // panic, so we cannot flush the buffer in panic().
    class Console {
    public:
        static Console& instance();

        Console();
        ~Console();

        Console(Console const&) = delete;
        Console(Console&&) = delete;

        void write_line(StringView s) {
            write(s.data(), s.length());
            write('\n');
        }

        void write(char c);
        void write(char const* s, usize len);
        void write(char const* begin, char const* end) {
            write(begin, static_cast<usize>(end - begin));
        }

        template <usize N>
        void write(char const (&s)[N]) {
            write(s, N);
        }

        void write(StringView s) { write(s.data(), s.length()); }

        void write_n(char c, usize n) {
            for (usize i = 0; i < n; i++) {
                write(c);
            }
        }

        void flush();

    private:
        void copy_to_buffer(char const* p, usize len);
        void output(char const* p, usize len);

        char* begin_;
        char* end_;
        char* cur_;
    };
}  // namespace spargel::base
