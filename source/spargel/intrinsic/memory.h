#pragma once

namespace spargel::intrinsic {
    inline int memcmp(void const* p, void const* q, unsigned long l) {
        return __builtin_memcmp(p, q, l);
    }
    inline void memcpy(void* dst, void const* src, unsigned long len) {
        __builtin_memcpy(dst, src, len);
    }
}  // namespace spargel::intrinsic
