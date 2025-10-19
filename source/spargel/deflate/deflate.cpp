#include "spargel/deflate/deflate.h"

#include "spargel/base/check.h"
#include "spargel/base/checked_convert.h"
#include "spargel/base/enum.h"
#include "spargel/base/inline_array.h"
#include "spargel/logging/logging.h"

//
#include <stdio.h>

namespace spargel::deflate {
    namespace {
#define BLOCK_TYPES_X()    \
    X(NoCompression, 0b00) \
    X(FixedHuffman, 0b01)  \
    X(DynamicHuffman, 0b11)
        enum class BlockType : u8 {
#define X(name, value) name = value,
            BLOCK_TYPES_X()
#undef X
        };
        BlockType bitsToBlockType(u8 x) {
            switch (x) {
#define X(name, value)                        \
    case base::toUnderlying(BlockType::name): \
        return BlockType::name;
                BLOCK_TYPES_X()
#undef X
            default:
                spargel_panic_here();
            }
        }
#undef BLOCK_TYPES_X
    }  // namespace
    void DeflateDecompressor::decompress(base::Span<base::Byte> input,
                                         base::Vector<base::Byte>& out) {
        stream_ = BitStream(input.begin(), input.end());
        while (true) {
            // TODO
            auto final_block = decompress_block(out);
            if (final_block) {
                break;
            }
        }
    }
    bool DeflateDecompressor::decompress_block(base::Vector<base::Byte>& out) {
        stream_.refill();
        u8 final_block = stream_.bit0();
        auto block_type = bitsToBlockType(stream_.bit21());
        stream_.advance_bits(3);
        switch (block_type) {
        case BlockType::NoCompression:
            plain_block(out);
            break;
        case BlockType::FixedHuffman:
            fixed_block(out);
            break;
        case BlockType::DynamicHuffman:
            break;
        }
        return final_block;
    }
    void DeflateDecompressor::fixed_block(base::Vector<base::Byte>& out) {
        // [RFC1951, Section 3.2.6]
        //
        // Fill the literal/length alphabet.
        //
        for (int i = 0; i < 144; i++) {
            litlen_symbols_length_[i] = 8;
        }
        for (int i = 144; i < 256; i++) {
            litlen_symbols_length_[i] = 9;
        }
        for (int i = 256; i < 280; i++) {
            litlen_symbols_length_[i] = 7;
        }
        for (int i = 280; i < 288; i++) {
            litlen_symbols_length_[i] = 8;
        }
        (void)out;
    }
    void DeflateDecompressor::plain_block(base::Vector<base::Byte>& out) {
        stream_.align_to_boundary();
        u16 len = stream_.consume_u16();
        u16 nlen = stream_.consume_u16();
        // TODO: Don't panic.
        //
        // NOTE:
        //   `nlen` is promoted to int in the expression `~nlen`.
        //   We need to mask the extended zeros.
        spargel_check(len == base::checkedConvert<u16>(~nlen & 0x00FF));
        stream_.copy(len, out);
    }
    //         least-significant bit
    // +--------+
    // |76543210|
    // +--------+
    //  most-significant bit
    //
    // All multi-byte numbers in the format described here are stored
    // with the least-significant byte first (at the lower memory address).
    //
    // In other words, if one were to print out the compressed data as a
    // sequence of bytes, starting with the first byte at the right margin and
    // proceeding to the left, with the most-significant bit of each byte on
    // the left as usual, one would be able to parse the result from right to
    // left, with fixed-width elements in the correct MSB-to-LSB order and
    // Huffman codes in bit-reversed order (i.e., with the first bit of the
    // code in the relative LSB position).
    void BitStream::refill() {
        while (bits_left_ < CONSUMABLE_BITS) {
            if (next_ == end_) [[unlikely]] {
                trailing_zero_++;
            } else {
                buffer_ |= static_cast<u64>(*next_) << bits_left_;
                next_++;
            }
            bits_left_ += 8;
        }
    }
    void BitStream::align_to_boundary() {
        u8 bytes = bits_left_ >> 3;
        spargel_check(trailing_zero_ <= bytes);
        next_ -= bytes - trailing_zero_;
        buffer_ = 0;
        bits_left_ = 0;
        trailing_zero_ = 0;
    }
    void BitStream::copy(usize n, base::Vector<base::Byte>& output) {
        spargel_check(n <= base::checkedConvert<usize>(end_ - next_));
        output.reserve(output.count() + n);
        memcpy(output.end(), next_, n);
        output.set_count(output.count() + n);
        next_ += n;
    }

    void DecodeTable::build(base::Span<u32> symbol_length,
                            u32 max_code_length) {
        constexpr int MAX_CODE_LENGTH = 15;
        spargel_check(max_code_length <= MAX_CODE_LENGTH);
        base::InlineArray<u32, MAX_CODE_LENGTH + 1> len_counts = {};
        // count the frequency of every lengths
        for (u32 i = 0; i < symbol_length.count(); i++) {
            len_counts[symbol_length[i]]++;
        }
        while (max_code_length > 1 && len_counts[max_code_length] == 0) {
            max_code_length--;
        }
        table_bits_ = max_code_length;

        u32 used_codespace = 0;

        base::InlineArray<u32, MAX_CODE_LENGTH + 1> offsets = {};
        offsets[0] = 0;
        for (u32 len = 1; len <= max_code_length; len++) {
            offsets[len] = offsets[len - 1] + len_counts[len - 1];
            used_codespace = (used_codespace << 1) + len_counts[len];
        }

        printf("offsets: ");
        for (u32 len = 1; len <= max_code_length; len++) {
            printf("%d ", offsets[len]);
        }
        printf("\n");

        printf("used_codespace: %d\n", used_codespace);
        printf("max_code_length: %d\n", max_code_length);

        // sort the symbols by (code length, symbol id).
        //
        // TODO: Provide the `sorted_syms` from outside, as its size should
        // match `symbol_length`.
        base::InlineArray<u32, 100 + 1> sorted_syms = {};
        for (u32 sym = 0; sym < symbol_length.count(); sym++) {
            sorted_syms[offsets[symbol_length[sym]]++] = sym;
        }

        printf("sorted syms: ");
        for (u32 sym = 0; sym < symbol_length.count(); sym++) {
            printf("%d ", sorted_syms[sym]);
        }
        printf("\n");
        logging::info("sorted syms: {}", sorted_syms.span());
    }

}  // namespace spargel::deflate
