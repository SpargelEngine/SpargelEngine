#pragma once

namespace spargel::base {

template <typename K, typename V>
struct HashMapEntry {
  K key;
  V value;
};

template <typename K, typename V>
class HashMap {
public:
  HashMap();
  ~HashMap();
};

}  // namespace spargel::base
