#pragma once

#define likely(x)   __builtin_expect(x, 1)
#define unlikely(x) __builtin_expect(x, 0)

inline size_t __bsf(size_t v) {
  return _tzcnt_u64(v);
}

inline size_t __bscf(size_t& v) {
  size_t i = __builtin_ctz(v);
  v &= v-1;
  return i;
}

#define __aligned(n)  __attribute__((aligned (n)))
#define __forceinline __attribute__((always_inline)) inline
