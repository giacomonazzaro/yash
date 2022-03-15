#pragma once
#include <array>
#include <vector>

#include "ext/pico_sha.h"

namespace yash {
using std::array;
using std::vector;
using byte = unsigned char;

using Hash = array<byte, 32>;  // 256-bit hash

template <typename T>
inline Hash make_hash(const T* data, size_t count) {
  auto hash = Hash{};
  picosha2::hash256((byte*)data, (byte*)(data + count), hash);
  return hash;
}

template <typename T>
inline Hash make_hash(const vector<T>& input) {
  return make_hash(input.data(), input.size());
}

template <typename T>
inline Hash make_hash(const T& value) {
  return make_hash(&value, 1);
}

static constexpr auto invalid_hash = Hash{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// Hash can be the key of std::unordered_map.
struct ArrayHasher {
  std::size_t operator()(const std::array<byte, 32>& a) const {
    // xor between four 64-bit chunks.
    std::size_t h = 0;
    for (int i = 0; i < 4; i++) h ^= *(size_t*)(&a[0] + sizeof(size_t) * i);
    return h;
  }
};

}  // namespace yash
