#pragma once

#include <immintrin.h>
#include <xmmintrin.h>

namespace simd {
  inline __m256 load(float x) {
    return _mm256_set1_ps(x);
  }

  inline __m256 load(const float* const x) {
    return _mm256_load_ps(x);
  }

  inline void store(const __m256& l, float* r) {
    _mm256_store_ps(r, l);
  }

  inline __m256 add(const __m256& l, const __m256& r) {
    return _mm256_add_ps(l, r);
  }
}
