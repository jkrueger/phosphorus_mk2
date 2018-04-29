#pragma once

#include <xmmintrin.h>

template<int N>
struct vector3_t
{};

#ifdef __SSE__
// TODO: implement 4 wide simd vectors
#endif

#ifdef __AVX2__

template<>
struct vector3_t<8> {
  union {
    __m256 v[3];
    struct {
      __m256 x, y, z;
    };
  };

  inline vector3_t()
  {}

  inline vector3_t(
    const float* _x
  , const float* _y
  , const float* _z)
  {
    x = _mm256_load_ps(_x);
    y = _mm256_load_ps(_y);
    z = _mm256_load_ps(_z);
  }
};

namespace simd {
  template<int N>
  inline void store(const vector3_t<N>& v, float* x, float* y, float* z);

  template<>
  inline void store<8>(const vector3_t<8>& v, float* x, float* y, float* z) {
    _mm256_store_ps(x, v.x);
    _mm256_store_ps(y, v.y);
    _mm256_store_ps(z, v.z);
  }
}

#endif
