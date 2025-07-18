#pragma once

#include <spargel/base/algorithm.h>
#include <spargel/base/allocator.h>
#include <spargel/base/check.h>
#include <spargel/base/meta.h>
#include <spargel/base/tag_invoke.h>
#include <spargel/base/types.h>

namespace spargel::base {

    namespace _array_storage {

        // `ArrayStorage<T>` provides storage for an array of `T`.
        //
        // Note:
        //     - It doesn't make sense to copy `ArrayStorage` unless `T` is trivially copyable.
        //       For simplicity, copy is banned unconditionally.
        //     - The status of the each slot is not tracked.
        //       So there is no way to swap two `ArrayStorage` with different allocators.
        //
        // TODO:
        //     - Rewrite to `OptionalArray`.
        //
        template <typename T>
        class ArrayStorage {
        public:
            ArrayStorage() = default;

            // Intialize with specified capacity.
            //
            // Parameters:
            //     - `count` is the required capacity
            //
            ArrayStorage(usize count) : _count{count} {
                if (count > 0)
                    _data = static_cast<Byte*>(default_allocator()->allocate(count * sizeof(T)));
            }

            // Copy constructor is removed.
            ArrayStorage(ArrayStorage const&) = delete;
            ArrayStorage& operator=(ArrayStorage const&) = delete;

            // Move is cheap.
            ArrayStorage(ArrayStorage&& other) : _count{other._count}, _data{other._data} {
                other._count = 0;
                other._data = nullptr;
            }
            ArrayStorage& operator=(ArrayStorage&& other) {
                ArrayStorage tmp(base::move(other));
                base::swap(*this, tmp);
                return *this;
            }

            ~ArrayStorage() {
                if (_data != nullptr) {
                    default_allocator()->free(_data, _count * sizeof(T));
                }
            }

            // Get the number of slots provided.
            usize getCount() const { return _count; }

            // Get the pointer to a slot by index.
            //
            // Parameters:
            //     - `i` is the index.
            //
            T* getPtr(usize i) {
                spargel_check(i < _count);
                return reinterpret_cast<T*>(_data + i * sizeof(T));
            }
            T const* getPtr(usize i) const {
                spargel_check(i < _count);
                return reinterpret_cast<T const*>(_data + i * sizeof(T));
            }

            T* begin() { return reinterpret_cast<T*>(_data); }
            T const* begin() const { return reinterpret_cast<T const*>(_data); }

            // Access the object by index.
            //
            // Parameters:
            //     - `i` is the index.
            //
            T& operator[](usize i) {
                spargel_check(i < _count);
                return *getPtr(i);
            }
            T const& operator[](usize i) const {
                spargel_check(i < _count);
                return *getPtr(i);
            }

            // Exchange storage.
            friend void tag_invoke(tag<swap>, ArrayStorage& lhs, ArrayStorage& rhs) {
                swap(lhs._count, rhs._count);
                swap(lhs._data, rhs._data);
            }

        private:
            usize _count = 0;
            // Storage is explicitly provided via a `Byte` array.
            Byte* _data = nullptr;
        };

    }  // namespace _array_storage

    using _array_storage::ArrayStorage;

}  // namespace spargel::base
