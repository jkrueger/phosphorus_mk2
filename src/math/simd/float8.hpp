#pragma once

#include <immintrin.h>
#include <xmmintrin.h>

namespace simd {
  template<int N>
  struct float_t
  {};

  template<>
  struct float_t<8> {
    typedef __m256 type;

    static inline __m256 gather(const float* const p, const __m256i& i) {
      return _mm256_i32gather_ps(p, i, 4);
    }
  };

  inline __m256 load(float x) {
    return _mm256_set1_ps(x);
  }

  inline __m256 load(const float* const x) {
    return _mm256_load_ps(x);
  }

  inline void store(const __m256& l, float* r) {
    _mm256_store_ps(r, l);
  }

  inline __m256 msub(const __m256& a, const __m256& b, const __m256& c) {
    return _mm256_fmsub_ps(a, b, c);
  }

  inline __m256 madd(const __m256& a, const __m256& b, const __m256& c) {
    return _mm256_fmadd_ps(a, b, c);
  }

  inline __m256 add(const __m256& l, const __m256& r) {
    return _mm256_add_ps(l, r);
  }

  inline __m256 sub(const __m256& l, const __m256& r) {
    return _mm256_sub_ps(l, r);
  }
  
  inline __m256 mul(const __m256& l, const __m256& r) {
    return _mm256_mul_ps(l, r);
  }

  inline __m256 div(const __m256& l, const __m256& r) {
    return _mm256_div_ps(l, r);
  }

  inline __m256 min(const __m256& l, const __m256& r) {
    return _mm256_min_ps(l, r);
  }

  inline __m256 max(const __m256& l, const __m256& r) {
    return _mm256_max_ps(l, r);
  }

  inline __m256 eq(const __m256& l, const __m256& r) {
    return _mm256_cmp_ps(l, r, _CMP_EQ_OS);
  }

  inline __m256 lt(const __m256& l, const __m256& r) {
    return _mm256_cmp_ps(l, r, _CMP_LT_OS);
  }

  inline __m256 lte(const __m256& l, const __m256& r) {
    return _mm256_cmp_ps(l, r, _CMP_LE_OS);
  }

  inline __m256 gt(const __m256& l, const __m256& r) {
    return _mm256_cmp_ps(l, r, _CMP_GT_OS);
  }

  inline __m256 gte(const __m256& l, const __m256& r) {
    return _mm256_cmp_ps(l, r, _CMP_GE_OS);
  }

  inline __m256 _or(const __m256& l, const __m256& r) {
    return _mm256_or_ps(l, r);
  }

  inline __m256 _and(const __m256& l, const __m256& r) {
    return _mm256_and_ps(l, r);
  }

  inline __m256 andnot(const __m256& l, const __m256& r) {
    return _mm256_andnot_ps(l, r);
  }

  inline __m256 select(const __m256& m, const __m256& l, const __m256& r) {
    return _mm256_blendv_ps(l, r, m);
  }

  inline size_t movemask(const __m256& mask) {
    return _mm256_movemask_ps(mask);
  }

  inline __m256 rcp(const __m256& x) {
    return _mm256_rcp_ps(x);
  }
}
