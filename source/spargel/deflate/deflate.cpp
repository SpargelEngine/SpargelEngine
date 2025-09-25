#include "spargel/deflate/deflate.h"

#include "spargel/base/check.h"
#include "spargel/base/checked_convert.h"
#include "spargel/base/enum.h"

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
        // while (true) {
        // TODO
        decompressBlock(out);
        // }
    }
    void DeflateDecompressor::decompressBlock(base::Vector<base::Byte>& out) {
        stream_.refill();
        [[maybe_unused]]
        u8 final_block = stream_.bit0();
        auto block_type = bitsToBlockType(stream_.bit21());
        stream_.advanceBits(3);
        if (block_type == BlockType::NoCompression) {
            plainBlock(out);
            return;
        }
        // TODO
    }
    void DeflateDecompressor::plainBlock(base::Vector<base::Byte>& out) {
        stream_.alignToBoundary();
        u16 len = stream_.consumeU16();
        u16 nlen = stream_.consumeU16();
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
    void DeflateDecompressor::BitStream::refill() {
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
    void DeflateDecompressor::BitStream::alignToBoundary() {
        u8 bytes = bits_left_ >> 3;
        spargel_check(trailing_zero_ <= bytes);
        next_ -= bytes - trailing_zero_;
        buffer_ = 0;
        bits_left_ = 0;
        trailing_zero_ = 0;
    }
    void DeflateDecompressor::BitStream::copy(
        usize n, base::Vector<base::Byte>& output) {
        spargel_check(n <= base::checkedConvert<usize>(end_ - next_));
        output.reserve(output.count() + n);
        memcpy(output.end(), next_, n);
        output.set_count(output.count() + n);
        next_ += n;
    }

}  // namespace spargel::deflate
