#include <spargel/base/allocator.h>
#include <spargel/base/assert.h>

// libc
#include <stdlib.h>

void* operator new(usize size, usize align) noexcept {
    return spargel::base::default_allocator()->allocate(size);
}

void* operator new[](usize size, usize align) noexcept {
    return spargel::base::default_allocator()->allocate(size);
}

void operator delete(void* ptr, usize size, usize align) noexcept {
    spargel::base::default_allocator()->free(ptr, size);
}

void operator delete[](void* ptr, usize size, usize align) noexcept {
    spargel::base::default_allocator()->free(ptr, size);
}

void* operator new(usize size, usize align, spargel::base::Allocator* alloc) noexcept {
    return alloc->allocate(size);
}

namespace spargel::base {

    Allocator* default_allocator() { return LibCAllocator::getInstance(); }

    LibCAllocator* LibCAllocator::getInstance() {
        static LibCAllocator inst;
        return &inst;
    }

    void* LibCAllocator::allocate(usize size) {
        spargel_assert(size > 0);
        return ::malloc(size);
    }

    void* LibCAllocator::resize(void* ptr, usize old_size, usize new_size) {
        spargel_assert(ptr != nullptr);
        spargel_assert(old_size > 0);
        spargel_assert(new_size > 0);
        return ::realloc(ptr, new_size);
    }

    void LibCAllocator::free(void* ptr, usize size) {
        spargel_assert(ptr != nullptr);
        spargel_assert(size > 0);
        ::free(ptr);
    }

}  // namespace spargel::base
