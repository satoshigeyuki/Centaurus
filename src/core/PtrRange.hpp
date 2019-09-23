#pragma once

namespace Centaurus
{

namespace detail
{

template <typename A>
struct ConstPtrRange {
  const A * const ptr;
  const std::size_t n;
  ConstPtrRange() : ptr(nullptr), n(0) {}
  ConstPtrRange(const A *ptr, std::size_t n) : ptr(ptr), n(n) {}
  const A* begin()  const { return ptr; }
  const A* cbegin() const { return ptr; }
  const A* end()    const { return ptr + n; }
  const A* cend()   const { return ptr + n; }
  const A& operator[](std::size_t i) const { return ptr[i]; }
  std::size_t size() const { return n; }
};

}
}
