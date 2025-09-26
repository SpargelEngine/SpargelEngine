#pragma once

#include "spargel/base/check.h"
#include "spargel/base/span.h"
#include "spargel/base/types.h"
#include "spargel/intrinsic/memory.h"

namespace spargel::png {
    // [PNG-3, Section 7.1]
    //     All integers that require more than one byte shall be in network byte
    //     order.
    class PNGDecoder {
    public:
        enum class DecodeStatus {
            success,
            bad_format,
            early_eof,
            wrong_signature,
        };

        PNGDecoder(u8 const* begin, u8 const* end) : begin_{begin}, end_{end} {}
        explicit PNGDecoder(base::Span<u8> bytes)
            : PNGDecoder(bytes.begin(), bytes.end()) {}

        DecodeStatus decode() {
            DecodeStatus result;
            if (result = consumeSignature(); result != DecodeStatus::success) {
                return result;
            }
            return DecodeStatus::success;
        }

        // [PNG-3, Section 5.2]
        //     The first eight bytes of a PNG datastream always contain the
        //     following hexadecimal values:
        //         89 50 4E 47 0D 0A 1A 0A
        //
        DecodeStatus consumeSignature() {
            constexpr u8 SIGNATURE[] = {
                0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A,
            };
            static_assert(sizeof(SIGNATURE) == 8);
            if (!remainsAtLeast(sizeof(SIGNATURE))) {
                return DecodeStatus::early_eof;
            }
            if (intrinsic::memcmp(begin_, SIGNATURE, sizeof(SIGNATURE)) != 0) {
                return DecodeStatus::wrong_signature;
            }
            advance(sizeof(SIGNATURE));
            return DecodeStatus::success;
        }

    private:
        // whether the byte stream has at least `n` bytes.
        bool remainsAtLeast(usize n) { return begin_ + n < end_; }
        void advance(usize n) { begin_ += n; }
        u8 peek() {
            spargel_check(begin_ < end_);
            spargel_check(begin_);
            return *begin_;
        }

        u8 const* begin_;
        u8 const* end_;
    };
}  // namespace spargel::png
