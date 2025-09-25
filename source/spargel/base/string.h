#pragma once

// TODO: Move UTF-8 specific code into `UnicodeString`. Make
// `base::String` a very basic string type that is used for calling
// system apis.

#include "spargel/base/algorithm.h"
#include "spargel/base/allocator.h"
#include "spargel/base/assert.h"
#include "spargel/base/check.h"
#include "spargel/base/hash.h"
#include "spargel/base/string_view.h"
#include "spargel/base/tag_invoke.h"
#include "spargel/base/types.h"
#include "spargel/base/vector.h"

// libc
#include <string.h>

namespace spargel::base {
    namespace _string {

        // A code point is a value in the Unicode codespace, i.e. an integer in
        // the range [0, 10ffff]. A Unicode scalar value is a code point that is
        // not a high-surrogate or a low-surrogate. In other words, a Unicode
        // scalar value is a code point in the range [0, d7ff] union [e000,
        // 10ffff].
        //
        // A `UnicodeScalar` represents a Unicode scalar value.
        //
        class UnicodeScalar {
        public:
            UnicodeScalar() = default;

            UnicodeScalar(u32 value) : _value{value} {
                spargel_assert(isValid(_value));
            }

            UnicodeScalar(UnicodeScalar const& other) : _value{other._value} {}
            UnicodeScalar& operator=(UnicodeScalar const& other) {
                _value = other._value;
                return *this;
            }

            u32 getValue() const { return _value; }

        private:
            static bool isValid(u32 value) {
                return (value <= 0x10ffff) &&
                       ((value <= 0xd7ff) || (value >= 0xe000));
            }

            u32 _value = 0;
        };

        // TODO: See above.
        //
        // A UTF-8 string.
        //
        //-------
        // UTF-8
        //
        // 0yyyzzzz
        // 110xxxyy 10yyzzzz
        // 1110wwww 10xxxxyy 10yyzzzz
        // 11110uvv 10vvwwww 10xxxxyy 10yyzzzz
        //
        class String {
        public:
            static String from_range(char const* begin, char const* end) {
                spargel_check(begin <= end);
                usize len = static_cast<usize>(end - begin);
                String result;
                result._bytes.reserve(len);
                result._bytes.set_count(len);
                memcpy(result._bytes.data(), begin, len);
                return result;
            }

            String() {}

            String(String const& other) : _bytes(other._bytes) {}
            String& operator=(String const& other) {
                String tmp(other);
                swap(*this, tmp);
                return *this;
            }

            String(String&& other) : _bytes(base::move(other._bytes)) {}
            String& operator=(String&& other) {
                String tmp(base::move(other));
                swap(*this, tmp);
                return *this;
            }

            ~String() = default;

            // migration from base::String
            explicit String(StringView view) {
                usize len = view.length();
                _bytes.reserve(len);
                _bytes.set_count(len);
                memcpy(_bytes.data(), view.data(), len);
            }
            /*explicit*/ String(char const* cstr) {
                usize len = strlen(cstr);
                _bytes.reserve(len);
                _bytes.set_count(len);
                memcpy(_bytes.data(), cstr, len);
            }
            String& operator=(char const* cstr) {
                String tmp(cstr);
                swap(*this, tmp);
                return *this;
            }
            explicit String(char ch) {
                _bytes.reserve(1);
                _bytes.set_count(1);
                _bytes.data()[0] = ch;
            }
            String& operator=(char ch) {
                String tmp(ch);
                swap(*this, tmp);
                return *this;
            }

            char& operator[](usize i) { return _bytes[i]; }
            char const& operator[](usize i) const { return _bytes[i]; }

            usize length() const { return _bytes.count(); }
            char* begin() { return _bytes.begin(); }
            char const* begin() const { return _bytes.begin(); }
            char* end() { return _bytes.end(); }
            char const* end() const { return _bytes.end(); }
            char* data() { return _bytes.data(); }
            char const* data() const { return _bytes.data(); }
            StringView view() const { return StringView(begin(), end()); }

            friend bool operator==(String const& lhs, String const& rhs) {
                if (lhs.length() != rhs.length()) return false;
                return memcmp(lhs.data(), rhs.data(), lhs.length()) == 0;
            }
            friend bool operator==(String const& lhs, StringView rhs) {
                if (lhs.length() != rhs.length()) return false;
                return memcmp(lhs.data(), rhs.data(), lhs.length()) == 0;
            }
            friend String operator+(String const& lhs, String const& rhs) {
                usize len = lhs.length() + rhs.length();
                String result;
                result._bytes.reserve(len);
                result._bytes.set_count(len);
                memcpy(result._bytes.data(), lhs.data(), lhs.length());
                memcpy(result._bytes.data() + lhs.length(), rhs.data(),
                       rhs.length());
                return result;
            }
            // not efficient
            friend String operator+(String const& s, char ch) {
                return s + String(ch);
            }
            friend String operator+(String const& s, char const* s2) {
                return s + String(s2);
            }

            String& operator+=(StringView view) {
                _bytes.reserve(length() + view.length());
                memcpy(_bytes.data() + length(), view.data(), view.length());
                return *this;
            }

            friend String operator+(String const& lhs, StringView rhs) {
                usize len = lhs.length() + rhs.length();
                String result;
                result._bytes.reserve(len);
                result._bytes.set_count(len);
                memcpy(result._bytes.data(), lhs.data(), lhs.length());
                memcpy(result._bytes.data() + lhs.length(), rhs.data(),
                       rhs.length());
                return result;
            }

            // Get the `i`-th byte.
            Byte getByte(usize i) const { return (Byte)_bytes[i]; }

            span<Byte> bytes() const { return _bytes.toSpan().asBytes(); }

            void resize(usize n) { _bytes.resize(n); }

            // usize getLength() const {
            //     usize len = 0;
            //     usize i = 0;
            //     while (i < length()) {
            //         Byte byte = bitCast<char, Byte>(_bytes[i]);
            //         if ((byte & 0b10000000) == 0) {
            //             i += 1;
            //         } else if ((byte & 0b11100000) == 0b11000000) {
            //             i += 2;
            //         } else if ((byte & 0b11110000) == 0b11100000) {
            //             i += 3;
            //         } else if ((byte & 0b11111000) == 0b11110000) {
            //             i += 4;
            //         } else {
            //             spargel_panic_here();
            //         }
            //         len += 1;
            //     }
            //     // spargel_assert(i == _bytes.count());
            //     return len;
            // }

            // Get the unicode scalar containing the `i`-th byte.
            // u32 getScalarAtByte(usize i) {
            //     spargel_panic_here();
            // }

            // unsafe
            void appendByte(Byte b) { _bytes.emplace((char)b); }

            friend void tag_invoke(tag<swap>, String& lhs, String& rhs) {
                swap(lhs._bytes, rhs._bytes);
            }

            friend void tag_invoke(tag<hash>, HashRun& r, String const& s) {
                r.combine((u8 const*)s._bytes.data(), s.length());
            }

        private:
            vector<char> _bytes;
        };

        class CString {
        public:
            CString(char const* beg, char const* end) {
                spargel_check(beg <= end);
                len_ = static_cast<usize>(end - beg);
                data_ = reinterpret_cast<char*>(
                    default_allocator()->allocate(len_ + 1));
                ::memcpy(data_, beg, len_);
                data_[len_] = 0;
            }
            CString(StringView view) : CString(view.begin(), view.end()) {}
            CString(const String& s) : CString(s.begin(), s.end()) {}
            ~CString() { default_allocator()->free(data_, len_ + 1); }

            auto data() const -> char const* { return data_; }

        private:
            char* data_;
            usize len_;
        };

    }  // namespace _string

    using _string::CString;
    using _string::String;
    using string = String;

    String string_from_range(char const* begin, char const* end);
}  // namespace spargel::base

// # Unicode
//
// ## Glossary
//
// - code point: any value in the unicode codespace, i.e. the range of integers
// from 0 to 10ffff
// - unicode scalar value; any unicode code point except high-surrogate and
// low-surrogate code points, i.e. [0, d7ff] cup [e000, 10ffff]
//
// - utf-8 encoding form: the unicode encoding form that assigns each unicode
// scalar value to an unsigned byte sequence of one to four bytes in length.
//
// - extended grapheme cluster: the text between extended grapheme cluster
// boundaries as specified by unicode standard annex #29
//
