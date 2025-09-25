#pragma once

#include "spargel/base/algorithm.h"
#include "spargel/base/allocator.h"
#include "spargel/base/check.h"
#include "spargel/base/meta.h"
#include "spargel/base/object.h"
#include "spargel/base/span.h"
#include "spargel/base/tag_invoke.h"
#include "spargel/base/types.h"

// libc
#include <string.h>

namespace spargel::base {

    namespace vector_ {

        /// TODO:
        ///     - Implement iterators (with generation checking).
        ///
        /// Questions:
        ///     - Do iterators remain valid when the vector is moved?
        ///       => No.
        ///
        template <typename T>
        class Vector {
        public:
            Vector() {}

            Vector(Vector const& other) {
                usize cnt = other.count();
                if (cnt > 0) {
                    allocate(cnt);
                    copyRange(other.begin(), other.end());
                }
            }
            Vector& operator=(Vector const& other) {
                Vector tmp(other);
                swap(*this, tmp);
                return *this;
            }

            Vector(Vector&& other) { swap(*this, other); }
            Vector& operator=(Vector&& other) {
                Vector tmp(move(other));
                swap(*this, tmp);
                return *this;
            }

            ~Vector() {
                if (_begin != nullptr) {
                    destruct_items();
                    deallocate();
                }
            }

            bool empty() const { return _end == _begin; }

            // construct and push back
            template <typename... Args>
            void emplace(Args&&... args) {
                if (_end >= _capacity) [[unlikely]] {
                    grow(capacity() + 1);
                }
                construct_at(_end, forward<Args>(args)...);
                _end++;
            }

            void push(T const& t) { emplace(t); }

            void push(T&& t) { emplace(move(t)); }

            void pop() {
                spargel_check(_begin < _end);
                _end--;
            }

            void clear() {
                destruct_items();
                _end = _begin;
            }

            // Ensure that the vector can hold `n` elements without
            // reallocation.
            void reserve(usize n) {
                if (_begin + n > _capacity) {
                    grow(n);
                }
            }

            void resize(usize count) {
                if (count > this->count()) {
                    reserve(count);
                    while (this->count() < count) {
                        emplace(T{});
                    }
                } else {
                    while (this->count() > count) {
                        pop();
                    }
                }
            }

            void resize(usize count, const T& value) {
                if (count > this->count()) {
                    reserve(count);
                    while (this->count() < count) {
                        emplace(value);
                    }
                } else {
                    while (this->count() > count) {
                        pop();
                    }
                }
            }

            // Erase the `i`-th object by swapping it to the end.
            // This guarantees no rellocation.
            void eraseFast(usize i) {
                spargel_check(i < count());
                auto p = _end - 1;
                swap(_begin[i], *p);
                destruct_at(p);
                _end--;
            }

            template <typename F>
            void eraseIfFast(F&& f) {
                usize i = 0;
                while (i < count()) {
                    if (!f(_begin[i])) {
                        i++;
                        continue;
                    }
                    eraseFast(i);
                }
            }

            template <typename F, typename D>
            void eraseIfFastWithDeleter(F&& f, D&& d) {
                usize i = 0;
                while (i < count()) {
                    if (!f(_begin[i])) {
                        i++;
                        continue;
                    }
                    auto p = _end - 1;
                    swap(_begin[i], *p);
                    d(*p);
                    _end--;
                }
            }

            // unsafe;
            void set_count(usize count) { _end = _begin + count; }

            T& operator[](usize i) {
                spargel_check(_begin + i < _end);
                return _begin[i];
            }
            T const& operator[](usize i) const {
                spargel_check(_begin + i < _end);
                return _begin[i];
            }

            T* data() { return _begin; }
            T const* data() const { return _begin; }
            usize count() const {
                spargel_check(_begin <= _end);
                return static_cast<usize>(_end - _begin);
            }
            usize capacity() const {
                spargel_check(_begin <= _capacity);
                return static_cast<usize>(_capacity - _begin);
            }

            T* begin() { return _begin; }
            T const* begin() const { return _begin; }
            T* end() { return _end; }
            T const* end() const { return _end; }

            Span<T> toSpan() const { return Span<T>(_begin, _end); }

            // Allocator* getAllocator() { return _alloc; }

            friend void tag_invoke(tag<swap>, Vector& lhs, Vector& rhs) {
                // todo: how to compare allocator?
                // if (lhs._alloc == rhs._alloc) [[likely]] {
                swap(lhs._begin, rhs._begin);
                swap(lhs._end, rhs._end);
                swap(lhs._capacity, rhs._capacity);
                // } else [[unlikely]] {
                //     spargel_panic_here();
                // }
            }

        private:
            // TODO: max size
            usize nextCapacity(usize count) {
                auto new_capacity = capacity() * 2;
                if (new_capacity < count) {
                    new_capacity = count;
                }
                return new_capacity;
            }

            void grow(usize need) {
                usize old_count = count();
                auto new_capacity = nextCapacity(need);
                T* new_begin = nullptr;
                // TODO
#if spargel_has_builtin(__is_trivially_copyable)
                if constexpr (__is_trivially_copyable(T)) {
                    if (_begin == nullptr) {
                        new_begin =
                            static_cast<T*>(default_allocator()->allocate(
                                sizeof(T) * new_capacity));
                    } else {
                        new_begin = static_cast<T*>(default_allocator()->resize(
                            _begin, capacity() * sizeof(T),
                            sizeof(T) * new_capacity));
                    }
                } else
#endif
                {

                    new_begin = static_cast<T*>(default_allocator()->allocate(
                        sizeof(T) * new_capacity));
                    if (_begin != nullptr) [[likely]] {
#if spargel_has_builtin(__builtin_is_cpp_trivially_relocatable)
                        if constexpr (__builtin_is_cpp_trivially_relocatable(
                                          T)) {
                            memcpy(new_begin, _begin, old_count * sizeof(T));
                        } else
// #elif spargel_has_builtin(__is_trivially_copyable)
//                     if constexpr (__is_trivially_copyable(T)) {
//                         memcpy(new_begin, _begin, old_count * sizeof(T));
//                     } else
#endif
                        {
                            move_items(new_begin);
                            destruct_items();
                        }
                        deallocate();
                    }
                }
                _begin = new_begin;
                _end = _begin + old_count;
                _capacity = _begin + new_capacity;
            }

            void move_items(T* ptr) {
                for (T* iter = _begin; iter < _end; iter++, ptr++) {
                    construct_at(ptr, move(*iter));
                }
            }

            void destruct_items() {
                for (T* iter = _begin; iter < _end; iter++) {
                    destruct_at(iter);
                }
            }

            void allocate(usize capacity) {
                _begin = static_cast<T*>(
                    default_allocator()->allocate(sizeof(T) * capacity));
                _end = _begin;
                _capacity = _begin + capacity;
            }

            void deallocate() {
                default_allocator()->free(_begin, sizeof(T) * capacity());
            }

            void copyRange(T const* begin, T const* end) {
                for (; begin < end; begin++, _end++) {
                    construct_at(_end, *begin);
                }
            }

            T* _begin = nullptr;
            T* _end = nullptr;
            T* _capacity = nullptr;
            // Allocator* _alloc = default_allocator();
        };

    }  // namespace vector_

    using vector_::Vector;

    // template <typename T>
    // using vector = Vector<T>;

}  // namespace spargel::base
