#pragma once

#include <unordered_map>

#include "ext/robin_hood.h"
#include "hash.h"
#include <view.h>

namespace yash {
#if 1
template <typename Key, typename Value>
using hash_map = robin_hood::unordered_flat_map<Key, Value>;
template <typename Key, typename Value>
using hash_set = robin_hood::unordered_flat_set<Key, Value>;
#else
template <typename Key, typename Value>
using hash_map = std::unordered_map<Key, Value>;
template <typename Key, typename Value>
using hash_set = std::unordered_set<Key, Value>;
#endif

using std::unordered_map;

struct Data_Table {
  unordered_map<Hash, vector<byte>, ArrayHasher> map = {};

  template <typename T>
  inline const T& get(const Hash& hash) const {
    static auto default_value = T{};
    auto        it            = map.find(hash);
    if (it == map.end()) return default_value;
    return *(T*)it->second.data();
  }

  template <typename T>
  inline const view<T> get_view(const Hash& hash) const {
    auto it = map.find(hash);
    if (it == map.end()) return {};
    auto& bytes = it->second;
    return view<T>((T*)&bytes[0], bytes.size() / sizeof(T));
  }

  template <typename T>
  inline bool set(const Hash& hash, const T& value) {
    static auto default_value = T{};
    if (memcmp(&value, &default_value, sizeof(T)) == 0) return false;
    map[hash] = vector<byte>((byte*)&value, (byte*)&value + sizeof(T));
    return true;
  }

  template <typename T>
  inline Hash maybe_add(const view<T>& value) {
    if (value.empty()) return invalid_hash;
    auto hash = make_hash(value.data, value.count);
    auto it   = map.find(hash);
    if (it != map.end()) {
      return it->first;
    } else {
      auto bytes = vector<byte>(
          (byte*)value.data, (byte*)(value.data + value.count));
      map.insert(it, {hash, bytes});
      return hash;
    }
  }

  template <typename T>
  inline Hash maybe_add(const vector<T>& value) {
    return maybe_add(view<const T>(value.data(), value.size()));
  }

  template <typename T>
  inline Hash maybe_add(const T& value) {
    return maybe_add(view<const T>{&value, 1});
  }
};

}  // namespace yash
