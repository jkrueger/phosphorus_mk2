#pragma once

#include <stdexcept>

/**
 * Most allocations in the rendering pipeline are sequential
 * and only last for one pipeline stage, so we organize memory
 * allocation as a stack, instead of husing the heap directly
 *
 */
struct allocator_t {
  char*  mem;
  char*  pos;
  size_t size;

  inline allocator_t(size_t size)
    : size(size)
  {
    pos = mem = new char[size]; 
  }

  inline ~allocator_t() {
    delete[] mem;
    mem = pos = nullptr;
  }

  inline char* allocate(size_t bytes) {
    if (used() + bytes > size) {
      throw std::runtime_error("Out of memory");
    }
    char* out = pos;
    pos += bytes;
    
    return out;
  }

  inline void reset() {
    pos = mem;
  }

  inline size_t used() const {
    return (pos - mem);
  }
};

struct allocator_scope_t {
  allocator_t& a;
  char*        start;

  inline allocator_scope_t(allocator_t& a)
    : a(a), start(a.pos)
  {}

  inline ~allocator_scope_t() {
    a.pos = start;
  }
};

inline void* operator new (size_t s, allocator_t& a) {
  return a.allocate(s);
}

inline void* operator new[] (size_t s, allocator_t& a) {
  return a.allocate(s);
}
