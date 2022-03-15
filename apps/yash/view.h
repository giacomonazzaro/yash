#pragma once

#include <stdlib.h>

#include <cassert>
#include <vector>

using int64 = long long;

template <typename Type>
struct view {
  Type*  data  = nullptr;
  size_t count = 0;

                     operator Type*() const { return data; }
  inline Type&       operator[](size_t i) { return data[i]; }
  inline const Type& operator[](size_t i) const { return data[i]; }
  inline size_t      size() const { return count; }
  inline bool        empty() const { return count == 0; }
  inline const Type& back() const { return data[count - 1]; }
  inline Type&       back() { return data[count - 1]; }
  inline const Type* begin() const { return data; }
  inline const Type* end() const { return data + count; }
  inline Type*       begin() { return data; }
  inline Type*       end() { return data + count; }

  view() {}
  view(Type* data, size_t count) : data(data), count(count) {}
  view(view&& rhs)      = default;
  view(const view& rhs) = default;
  view(const std::vector<Type>& rhs)
      : data((Type*)rhs.data()), count(rhs.size()){};
  view& operator=(const view& rhs) = default;  // debetable...

  inline void operator+=(const Type& e) { data[count++] = e; }

  inline view<Type>&& clear() {
    count = 0;
    return static_cast<view<Type>&&>(*this);
  }

  inline view<Type> slice(int64 from, int64 to) {
    while (to < 0) to += count;
    while (from < 0) from += count;
    assert(from <= count);
    assert(to <= count);
    assert(from <= to);
    return view<Type>{data + from, size_t(to - from)};
  }

  inline const view<const Type> slice(int64 from, int64 to) const {
    while (to < 0) to += count;
    while (from < 0) from += count;
    assert(from <= count);
    assert(to <= count);
    assert(from <= to);
    return view<const Type>{data + from, to - from};
  }

  inline view<Type> slice(int64 from = 0) { return view(from, count); }

  inline const view<const Type> slice(int64 from) const {
    return view(from, count);
  }
};

template <typename Type>
int64 find(const view<Type>& array, const Type& value) {
  for (auto i = 0; i < array.count; i++) {
    if (array[i] == value) return i;
  }
  return -1;
}

// fill with a constant value
template <typename Type>
inline void fill(view<Type>& array, const Type& value) {
  for (auto& v : array) v = value;
}

// fill with values returned by a function f: (size_t) -> Type
template <typename Type, typename Function>
inline void fill(view<Type>& array, Function f) {
  for (auto i = 0; i < array.count; i++) array[i] = f(i);
}

#include <string>  // std::to_string
#include <typeinfo>  // Is it slow to compile?

template <typename Type>
void print(const view<Type>& array) {
  auto name = typeid(Type).name();
  printf("view<%s>(%p, %ld) {\n", name, array.data, array.count);
  for (size_t i = 0; i < array.count; i++) {
    printf("  [%ld]: %s\n", i, std::to_string(array[i]).c_str());
  }
  printf("}\n");
}
