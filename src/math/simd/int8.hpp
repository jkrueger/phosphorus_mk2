#pragma once

#include <immintrin.h>
#include <xmmintrin.h>

#include <stdint.h>

namespace simd {
  template<int N>
  struct int32_t
  {};

  template<>
  struct int32_t<8> {
    typedef __m256i type;
  };

  inline __m256i load(::int32_t x) {
    return _mm256_set1_epi32(x);
  }

  inline __m256i load(::int32_t* const x) {
    return _mm256_load_si256((__m256i * const) x);
  }

  inline void store(const __m256i& l, ::int32_t* r) {
    _mm256_store_si256((__m256i*) r, l);
  }
}
