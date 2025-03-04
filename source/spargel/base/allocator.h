#pragma once

#include <spargel/base/logging.h>
#include <spargel/base/meta.h>
#include <spargel/base/object.h>
#include <spargel/base/panic.h>
#include <spargel/base/types.h>

// libc
#include <string.h>

// the default placement new cannot be overrided
#include <new>  // IWYU pragma: export

namespace spargel::base {

    class Allocator {
    public:
        virtual ~Allocator() = default;

        // requires: size > 0
        virtual void* allocate(usize size) = 0;
        // requires: ptr != nullptr, old_size > 0, new_size > 0, old_size < new_size
        virtual void* resize(void* ptr, usize old_size, usize new_size) = 0;
        // requires: ptr != nullptr, size > 0
        virtual void free(void* ptr, usize size) = 0;

        template <typename T, typename... Args>
        T* allocObject(Args&&... args) {
            T* ptr = static_cast<T*>(allocate(sizeof(T)));
            construct_at<T>(ptr, forward<Args>(args)...);
            return ptr;
        }

        template <typename T>
        void freeObject(T* ptr) {
            ptr->~T();
            free(ptr, sizeof(T));
        }
    };

    Allocator* default_allocator();

    class LibCAllocator final : public Allocator {
    public:
        static LibCAllocator* getInstance();

        void* allocate(usize size) override;
        void* resize(void* ptr, usize old_size, usize new_size) override;
        void free(void* ptr, usize size) override;
    };

    class ArenaAllocator final : public Allocator {
    public:
        ArenaAllocator(usize size, Allocator* alloc) : _alloc{alloc} {
            _begin = reinterpret_cast<Byte*>(_alloc->allocate(size));
            _end = _begin + size;
            _cur = _begin;
        }
        ~ArenaAllocator() override { _alloc->free(_begin, _end - _begin); }

        void* allocate(usize size) override {
            if (_cur + size >= _end) [[unlikely]] {
                spargel_log_fatal("arena out of space");
                spargel_panic_here();
            }
            _cur += size;
            return _cur;
        }

        void* resize(void* ptr, usize old_size, usize new_size) override {
            if (_cur - old_size == ptr) {
                // this is the last allocation
                _cur += new_size - old_size;
                return ptr;
            }
            auto new_ptr = allocate(new_size);
            memcpy(new_ptr, ptr, old_size);
            return new_ptr;
        }

        void free(void* ptr, usize size) override {
            if (_cur - size == ptr) {
                _cur = reinterpret_cast<Byte*>(ptr);
            }
        }

    private:
        Byte* _begin;
        Byte* _end;
        Byte* _cur;
        Allocator* _alloc;
    };

}  // namespace spargel::base

void* operator new(usize size, usize align, spargel::base::Allocator* alloc) noexcept;
