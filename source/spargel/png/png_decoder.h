#pragma once

#include "spargel/base/check.h"
#include "spargel/base/format.h"
#include "spargel/base/span.h"
#include "spargel/base/string_view.h"
#include "spargel/base/types.h"
#include "spargel/intrinsic/memory.h"

// ntohl
#include <arpa/inet.h>

#include "spargel/logging/logging.h"

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
            if (result = decode_signature(); result != DecodeStatus::success) {
                return result;
            }
            while (begin_ < end_) {
                if (result = decode_chunk(); result != DecodeStatus::success) {
                    return result;
                }
            }
            return DecodeStatus::success;
        }

    private:
        // [PNG-3, Table 7]
        // Critical chunks
        //
        enum class KnownChunk {
            // unique; first chunk
            IHDR,
            // unique; before first IDAT
            PLTE,
            // many; consecutive
            IDAT,
            // unique; last
            IEND,
        };

        static char const* convert_name(KnownChunk name) {
            switch (name) {
            case KnownChunk::IHDR:
                return "IHDR";
            case KnownChunk::PLTE:
                return "PLTE";
            case KnownChunk::IDAT:
                return "IDAT";
            case KnownChunk::IEND:
                return "IEND";
            }
        }

        struct ChunkName {
            explicit ChunkName(u8 const* begin) {
                intrinsic::memcpy(data, begin, sizeof(data));
            }

            bool operator==(KnownChunk name) const {
                return intrinsic::memcmp(data, convert_name(name),
                                         sizeof(data)) == 0;
            }

            base::StringView stringView() const {
                return base::StringView{data, 4};
            }

            template <base::FormatTarget Target>
            friend void tag_invoke(base::tag<base::formatArg>, Target& target,
                                   base::detail::FormatString fmt,
                                   ChunkName name) {
                base::formatTo(target, fmt, name.stringView());
            }

            char data[4];
        };

        // [PNG-3, Section 5.2]
        //     The first eight bytes of a PNG datastream always contain the
        //     following hexadecimal values:
        //         89 50 4E 47 0D 0A 1A 0A
        //
        DecodeStatus decode_signature() {
            constexpr u8 SIGNATURE[] = {
                0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A,
            };
            static_assert(sizeof(SIGNATURE) == 8);
            if (!remains_at_least(sizeof(SIGNATURE))) {
                return DecodeStatus::early_eof;
            }
            if (intrinsic::memcmp(begin_, SIGNATURE, sizeof(SIGNATURE)) != 0) {
                return DecodeStatus::wrong_signature;
            }
            advance(sizeof(SIGNATURE));
            return DecodeStatus::success;
        }

        // [PNG-3, Section 5.3]
        //     Each chunk consists of three or four fields.
        //         length      ; chunk type ; chunk data ; crc
        //         length (=0) ; chunk type ; crc
        //
        //     `length` is a PNG four-byte unsigned integer giving the number of
        //     bytes in the chunk's data field.
        //
        //     `chunk type` is a sequence of four letters [a-zA-Z].
        //
        //     `crc` is a four-byte CRC calculated on the preceding bytes in the
        //     chunk, including the chunk type field and chunk data fields, but
        //     not including the length field.
        //
        DecodeStatus decode_chunk() {
            constexpr int MIN_SIZE = 12;
            if (!remains_at_least(MIN_SIZE)) {
                return DecodeStatus::early_eof;
            }
            u32 length = read_u32_be();
            auto chunk_name = read_chunk_name();
            logging::info("chunk: {}", chunk_name);
            if (!remains_at_least(length + 4)) {
                return DecodeStatus::early_eof;
            }
            advance(length);
            u32 crc = read_u32_be();
            (void)crc;
            return DecodeStatus::success;
        }

        // whether the byte stream has at least `n` bytes.
        bool remains_at_least(usize n) { return begin_ + n <= end_; }
        void advance(usize n) { begin_ += n; }
        // the caller should ensure enough space
        u32 read_u32_be() {
            u32 tmp;
            intrinsic::memcpy(&tmp, begin_, sizeof(u32));
            advance(sizeof(u32));
            return ntohl(tmp);
        }
        ChunkName read_chunk_name() {
            ChunkName tmp{begin_};
            advance(4);
            return tmp;
        }

        u8 peek() {
            spargel_check(begin_ < end_);
            spargel_check(begin_);
            return *begin_;
        }

        u8 const* begin_;
        u8 const* end_;
    };
}  // namespace spargel::png
