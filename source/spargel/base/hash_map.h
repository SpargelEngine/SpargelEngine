#pragma once

#include "spargel/base/algorithm.h"
#include "spargel/base/array_storage.h"
#include "spargel/base/assert.h"
#include "spargel/base/hash.h"
#include "spargel/base/meta.h"
#include "spargel/base/tag_invoke.h"
#include "spargel/base/vector.h"

namespace spargel::base {

    namespace _hash_map {

        // HashMap
        //
        // `K` is the type of keys, which should be trivially copyable.
        //
        template <typename K, typename T>
        class HashMap {
        public:
            HashMap() = default;

            HashMap(HashMap const& other)
                : _capacity{other._capacity},
                  _count{other._count},
                  _status(other._status),
                  _keys(other._capacity),
                  _values(other._capacity) {
                spargel_assert(_capacity == _status.count());

                for (usize i = 0; i < _capacity; i++) {
                    if (_status[i] == SlotStatus::used) {
                        construct_at(_keys.getPtr(i), other._keys[i]);
                        construct_at(_values.getPtr(i), other._values[i]);
                    }
                }
            }
            HashMap& operator=(HashMap const& other) {
                HashMap tmp(other);
                swap(*this, tmp);
                return *this;
            }

            // FIXME:
            // ArrayStorage does not hava a default constructor
            HashMap(HashMap&& other) { swap(*this, other); }
            HashMap& operator=(HashMap&& other) {
                HashMap tmp(base::move(other));
                swap(*this, tmp);
                return *this;
            }

            ~HashMap() { destructItems(); }

            T* get(K const& key) {
                auto result = findSlot(key);
                if (result.has_key) {
                    return &_values[result.index];
                }
                return nullptr;
            }

            template <typename U>
            T const* get(U const& key) const {
                auto result = findSlot(key);
                if (result.has_key) {
                    return &_values[result.index];
                }
                return nullptr;
            }

            template <typename U, typename... Args>
            void set(U const& key, Args&&... args) {
                ensureSpace();

                FindResult r = findSlot(key);
                if (r.has_key) {
                    destruct_at(_values.getPtr(r.index));
                    construct_at(_values.getPtr(r.index),
                                 forward<Args>(args)...);
                } else {
                    _status[r.index] = SlotStatus::used;
                    construct_at(_keys.getPtr(r.index), key);
                    construct_at(_values.getPtr(r.index),
                                 forward<Args>(args)...);
                }

                _count++;
            }

            template <typename... Args>
            T& getOrConstruct(K const& key, Args&&... args) {
                FindResult result = findSlot(key);
                if (!result.has_key) {
                    _status[result.index] = SlotStatus::used;
                    construct_at(_keys.getPtr(result.index), key);
                    construct_at(_values.getPtr(result.index),
                                 forward<Args>(args)...);
                }
                return *_values.getPtr(result.index);
            }

            usize count() const { return _count; }

            friend void tag_invoke(tag<swap>, HashMap& lhs, HashMap& rhs) {
                base::swap(lhs._capacity, rhs._capacity);
                base::swap(lhs._count, rhs._count);
                base::swap(lhs._status, rhs._status);
                base::swap(lhs._keys, rhs._keys);
                base::swap(lhs._values, rhs._values);
            }

        private:
            enum class SlotStatus {
                free,
                used,
            };

            struct FindResult {
                bool has_key;
                usize index;
            };

            void grow() {
                usize new_cap = nextCapacity();

                base::Vector<SlotStatus> new_status;
                base::ArrayStorage<K> new_keys{new_cap};
                base::ArrayStorage<T> new_values{new_cap};
                new_status.reserve(new_cap);
                new_status.set_count(new_cap);
                for (usize i = 0; i < new_cap; i++) {
                    new_status[i] = SlotStatus::free;
                }

                for (usize i = 0; i < _capacity; i++) {
                    if (_status[i] == SlotStatus::used) {
                        usize new_i =
                            findFreeSlot(_keys[i], new_cap, new_status);

                        new_status[new_i] = SlotStatus::used;
                        construct_at<K>(new_keys.getPtr(new_i),
                                        base::move(_keys[i]));
                        construct_at<T>(new_values.getPtr(new_i),
                                        base::move(_values[i]));
                    }
                }

                destructItems();

                _capacity = new_cap;
                swap(_status, new_status);
                swap(_keys, new_keys);
                swap(_values, new_values);
            }

            usize findFreeSlot(K const& key, usize cap,
                               Vector<SlotStatus> const& status) {
                spargel_assert(cap > 0);

                u64 h = hash(key);
                usize i = h % cap;
                while (true) {
                    if (status[i] == SlotStatus::free) {
                        return i;
                    }
                    i = (i + 1) % cap;
                }
            }

            template <typename U>
            FindResult findSlot(U const& key) const {
                if (_capacity == 0) {
                    return FindResult(false, 0);
                }

                u64 h = hash(key);
                usize i = h % _capacity;
                while (true) {
                    if ((_status[i] == SlotStatus::used) && (_keys[i] == key)) {
                        return FindResult(true, i);
                    } else if (_status[i] == SlotStatus::free) {
                        return FindResult(false, i);
                    }
                    i = (i + 1) % _capacity;
                }
            }

            void destructItems() {
                auto p1 = _status.begin();
                auto p2 = _keys.begin();
                auto p3 = _values.begin();
                while (p1 < _status.end()) {
                    if (*p1 == SlotStatus::used) {
                        destruct_at(p2);
                        destruct_at(p3);
                    }
                    p1++;
                    p2++;
                    p3++;
                }
            }

            bool needToGrow() const { return (_capacity * 3 / 4) <= _count; }
            usize nextCapacity() const {
                return _capacity == 0 ? 8 : _capacity * 2;
            }
            void ensureSpace() {
                if (needToGrow()) grow();
            }

            usize _capacity = 0;
            usize _count = 0;
            base::Vector<SlotStatus> _status;
            base::ArrayStorage<K> _keys;
            base::ArrayStorage<T> _values;
        };

    }  // namespace _hash_map

    using _hash_map::HashMap;

}  // namespace spargel::base
