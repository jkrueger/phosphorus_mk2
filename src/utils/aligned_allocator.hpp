#pragma once

#include <cstdlib>
#include <type_traits>

template <typename T, std::size_t N>
class aligned_allocator {
public:
  //  type to allocate
  typedef T value_type;
  typedef T& reference;
  typedef const T& const_reference;
  typedef T* pointer;
  typedef const T* const_pointer;
  typedef size_t size_type;
  typedef ptrdiff_t difference_type;

  aligned_allocator()                                         = default;
  aligned_allocator(const aligned_allocator&)                 = default;
  aligned_allocator& operator=(const aligned_allocator&)      = default;
  aligned_allocator(aligned_allocator&&) noexcept             = default;
  aligned_allocator& operator=(aligned_allocator&&) noexcept  = default;
  
  template <typename U>
  aligned_allocator(const aligned_allocator<U, N>&) {}
  
  virtual ~aligned_allocator() noexcept = default;
  
  template <typename U>
  struct rebind {
    using other = aligned_allocator<U, N>;
  };

  T* allocate(std::size_t n) {
    // total size needs to be multiple of alignment
    auto size = n * sizeof(T);
    auto pad  = N - (size % N);

#ifdef aligned_alloc
    auto mem = aligned_alloc(N, size + (pad == N ? 0 : pad));
#else
    void* mem;
    posix_memalign(&mem, N, size + (pad == N ? 0 : pad));
#endif
    return reinterpret_cast<T*>(mem);
  }

  void deallocate(T* p, std::size_t) {
    free(p);
  }

  inline void construct(T* p, const T& value) { new (p) T(value); }
  inline void destroy(T* p) { p->~T(); }
};

template <typename T, typename U, std::size_t N>
constexpr bool operator==(
  const aligned_allocator<T, N>&
, const aligned_allocator<U, N>&)
{
  return true;
}

template <typename T, typename U, std::size_t N>
constexpr bool operator!=(
  const aligned_allocator<T, N>&
, const aligned_allocator<U, N>&) {
  return false;
}
