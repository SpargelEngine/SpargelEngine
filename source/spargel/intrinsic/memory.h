#pragma once

namespace spargel::intrinsic {
    int memcmp(void const* p, void const* q, unsigned long l) {
        return __builtin_memcmp(p, q, l);
    }
}  // namespace spargel::intrinsic
