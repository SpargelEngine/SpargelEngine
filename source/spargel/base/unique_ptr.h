#pragma once

#include <spargel/base/algorithm.h>
#include <spargel/base/allocator.h>
#include <spargel/base/meta.h>
#include <spargel/base/object.h>
#include <spargel/base/tag_invoke.h>
#include <spargel/base/types.h>

namespace spargel::base {

    namespace __unique_ptr {
        template <typename T>
        class unique_ptr {
        public:
            constexpr unique_ptr() = default;
            constexpr unique_ptr(nullptr_t) {}

            explicit constexpr unique_ptr(T* ptr) : _ptr{ptr} {}

            template <typename U>
                requires(is_convertible<U*, T*>)
            constexpr unique_ptr(unique_ptr<U>&& other) : _ptr{other.release()} {}

            template <typename U>
                requires(is_convertible<U*, T*>)
            constexpr unique_ptr& operator=(unique_ptr<U>&& other) {
                unique_ptr tmp(move(other));
                swap(*this, tmp);
                return *this;
            }

            unique_ptr(unique_ptr const&) = delete;

            ~unique_ptr() {
                if (_ptr != nullptr) {
                    default_allocator()->freeObject(_ptr);
                }
            }

            T* operator->() { return _ptr; }
            T const* operator->() const { return _ptr; }

            T* get() { return _ptr; }
            T const* get() const { return _ptr; }

            T* release() {
                T* p = _ptr;
                _ptr = nullptr;
                return p;
            }

            friend void tag_invoke(tag<swap>, unique_ptr& lhs, unique_ptr& rhs) {
                swap(lhs._ptr, rhs._ptr);
            }

        private:
            T* _ptr = nullptr;
        };
    }  // namespace __unique_ptr

    using __unique_ptr::unique_ptr;

    template <typename T, typename... Args>
    unique_ptr<T> make_unique(Args&&... args) {
        return unique_ptr<T>(default_allocator()->allocObject<T>(forward<Args>(args)...));
    }

}  // namespace spargel::base
