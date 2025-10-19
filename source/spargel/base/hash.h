#pragma once

#include "spargel/base/bit_cast.h"
#include "spargel/base/compiler.h"
#include "spargel/base/meta.h"
#include "spargel/base/tag_invoke.h"
#include "spargel/base/types.h"

//
#include <string.h>

// todo: now the code assumes little-endian
// use `__builtin_bswap64` (gcc/clang) or `_byteswap_uint64` (msvc) to swap
// endian

#if SPARGEL_IS_MSVC
#include <intrin.h>
#pragma intrinsic(_mul128)
#endif

namespace spargel::base {

    namespace __wyhash {

        inline u64 wyread3(u8 const* data, u64 len) {
            auto p = static_cast<u8 const*>(data);
            u64 a = p[0];
            u64 b = p[len >> 1];
            u64 c = p[len - 1];
            return (a << 56) | (b << 32) | c;
        }

        inline u64 wyread4(u8 const* data) {
            u32 v;
            memcpy(&v, data, 4);
            return v;
        }

        inline u64 wyread8(u8 const* data) {
            u64 v;
            memcpy(&v, data, 8);
            return v;
        }

        inline constexpr bool protect_mode = false;

        inline constexpr void wymul(u64& a, u64& b) {
#ifdef __SIZEOF_INT128__
            __uint128_t r = a;
            r *= b;
            // under O1, this is
            //   a single `mul` (x64)
            //   one `mul` and one `umulh` (arm64)
            //
            // two more `xor`/`eor` in protect_mode
            if (protect_mode) {
                a ^= (u64)r;
                b ^= (u64)(r >> 64);
            } else {
                a = (u64)r;
                b = (u64)(r >> 64);
            }
#elif SPARGEL_IS_MSVC
            /* FIXME: this is not regarded as constexpr
            __int64 h, l;
            l = _mul128(a, b, &h);
            a = l;
            b = h;
            */
#else
#error unimplemented
#endif
        }

        inline constexpr u64 wymix(u64 a, u64 b) {
            wymul(a, b);
            return a ^ b;
        }

        inline constexpr u64 secret0 = 0x2d358dccaa6c78a5;
        inline constexpr u64 secret1 = 0x8bb84b93962eacc9;
        inline constexpr u64 secret2 = 0x4b33a62ed433d4a3;

        inline constexpr u64 default_seed = 0xbdd89aa982704029;

        inline constexpr u64 wyhash(u8 const* data, u64 len, u64 seed) {
            u64 a;
            u64 b;

            auto p = data;

            if (len <= 16) [[likely]] {
                if (len >= 4) {
                    auto plast = p + len - 4;
                    // how about ((len&24)>>(len>>3))
                    auto delta = (len >> 3) << 2;
                    a = (wyread4(p) << 32) | wyread4(plast);
                    b = (wyread4(p + delta) << 32) | wyread4(plast - delta);
                } else if (len > 0) {
                    a = wyread3(data, len);
                    b = 0;
                } else {
                    a = 0;
                    b = 0;
                }
            } else {
                u64 i = len;
                if (i > 48) {
                    u64 seed1 = seed;
                    u64 seed2 = seed;
                    do {
                        seed =
                            wymix(wyread8(p) ^ secret0, wyread8(p + 8) ^ seed);
                        seed1 = wymix(wyread8(p + 16) ^ secret1,
                                      wyread8(p + 24) ^ seed1);
                        seed2 = wymix(wyread8(p + 32) ^ secret2,
                                      wyread8(p + 40) ^ seed2);
                        p += 48;
                        i -= 48;
                    } while (i >= 48);
                    seed ^= seed1 ^ seed2;
                }
                if (i > 16) {
                    seed = wymix(wyread8(p) ^ secret2,
                                 wyread8(p + 8) ^ seed ^ secret1);
                    if (i > 32) {
                        seed = wymix(wyread8(p + 16) ^ secret2,
                                     wyread8(p + 24) ^ seed);
                    }
                }
                a = wyread8(p + i - 16);
                b = wyread8(p + i - 8);
            }
            a ^= secret1;
            b ^= seed;
            wymul(a, b);
            return wymix(a ^ secret0 ^ len, b ^ secret1);
        }

    }  // namespace __wyhash

    class HashRun {
    public:
#if SPARGEL_IS_MSVC
        constexpr void combine(u8 v) {
            u8 b[1];
            b[0] = v;
            _hash = __wyhash::wyhash(b, 1, _hash);
        }
        constexpr void combine(u16 v) {
            u8 b[2];
            b[0] = v & 0xff;
            b[1] = (v >> 8) & 0xff;
            _hash = __wyhash::wyhash(b, 2, _hash);
        }
        constexpr void combine(u32 v) {
            u8 b[4];
            b[0] = v & 0xff;
            b[1] = (v >> 8) & 0xff;
            b[2] = (v >> 16) & 0xff;
            b[3] = (v >> 24) & 0xff;
            _hash = __wyhash::wyhash(b, 4, _hash);
        }
        constexpr void combine(u64 v) {
            u8 b[8];
            b[0] = v & 0xff;
            b[1] = (v >> 8) & 0xff;
            b[2] = (v >> 16) & 0xff;
            b[3] = (v >> 24) & 0xff;
            b[4] = (v >> 32) & 0xff;
            b[5] = (v >> 40) & 0xff;
            b[6] = (v >> 48) & 0xff;
            b[7] = (v >> 56) & 0xff;
            _hash = __wyhash::wyhash(b, 8, _hash);
        }
#else
        void combine(u8 v) {
            u8 b[1];
            memcpy(b, &v, 1);
            _hash = __wyhash::wyhash(b, 1, _hash);
        }
        void combine(u16 v) {
            u8 b[2];
            memcpy(b, &v, 2);
            _hash = __wyhash::wyhash(b, 2, _hash);
        }
        void combine(u32 v) {
            u8 b[4];
            memcpy(b, &v, 4);
            _hash = __wyhash::wyhash(b, 4, _hash);
        }
        void combine(u64 v) {
            u8 b[8];
            memcpy(b, &v, 8);
            _hash = __wyhash::wyhash(b, 8, _hash);
        }
#endif

        void combine(u8 const* data, u64 len) {
            _hash = __wyhash::wyhash(data, len, _hash);
        }

        template <typename T>
        void combine(T const& v);

        u64 result() const { return _hash; }

    private:
        u64 _hash = __wyhash::default_seed;
    };

    namespace __hash {
        struct hash {
            template <typename T>
            void operator()(HashRun& run, T&& v) const {
                tag_invoke(hash{}, run, forward<T>(v));
            }
            template <typename T>
            u64 operator()(T&& v) const {
                HashRun r;
                (*this)(r, forward<T>(v));
                return r.result();
            }
        };
        inline void tag_invoke(hash, HashRun& r, u8 v) { r.combine(v); }
        inline void tag_invoke(hash, HashRun& r, u16 v) { r.combine(v); }
        inline void tag_invoke(hash, HashRun& r, u32 v) { r.combine(v); }
        inline void tag_invoke(hash, HashRun& r, u64 v) { r.combine(v); }
        inline void tag_invoke(hash, HashRun& r, i8 v) {
            r.combine(bit_cast<i8, u8>(v));
        }
        inline void tag_invoke(hash, HashRun& r, i16 v) {
            r.combine(bit_cast<i16, u16>(v));
        }
        inline void tag_invoke(hash, HashRun& r, i32 v) {
            r.combine(bit_cast<i32, u32>(v));
        }
        inline void tag_invoke(hash, HashRun& r, i64 v) {
            r.combine(bit_cast<i64, u64>(v));
        }
    }  // namespace __hash

    inline constexpr __hash::hash hash{};

    template <typename T>
    void HashRun::combine(T const& v) {
        hash(*this, v);
    }

}  // namespace spargel::base
