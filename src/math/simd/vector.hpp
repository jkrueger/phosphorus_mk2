#pragma once

#include "float8.hpp"

#include <ImathVec.h>

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
  //union {
  //__m256 v[3];
  //struct {
      __m256 x, y, z;
  //};
  //};

  inline vector3_t()
  {}

  inline vector3_t(const vector3_t& cpy)
    : x(cpy.x), y(cpy.y), z(cpy.z)
  {}

  inline vector3_t(
      const __m256& _x
    , const __m256& _y
    , const __m256& _z)
    : x(_x), y(_y), z(_z)
  {}

  inline vector3_t(
    const float _x
  , const float _y
  , const float _z)
  {
    x = _mm256_set1_ps(_x);
    y = _mm256_set1_ps(_y);
    z = _mm256_set1_ps(_z);
  }

  inline vector3_t(const Imath::V3f& v)
    : vector3_t(v.x, v.y, v.z)
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

  /* loads a vector from an SOA data structure via gather instructions */
  inline vector3_t(
    const float* _x
  , const float* _y
  , const float* _z
  , const __m256i& idx)
  {
    x = _mm256_i32gather_ps(_x, idx, 4);
    y = _mm256_i32gather_ps(_y, idx, 4);
    z = _mm256_i32gather_ps(_z, idx, 4);
  }

  inline vector3_t rcp() const {
    return {_mm256_rcp_ps(x), _mm256_rcp_ps(y), _mm256_rcp_ps(z)};
  }

  inline __m256 dot(const vector3_t<8>& r) const {
    return simd::madd(x, r.x, simd::madd(y, r.y, simd::mul(z, r.z)));
  }

  inline vector3_t cross(const vector3_t& r) const {
    vector3_t<8> out = {
      simd::msub(y, r.z, simd::mul(z, r.y)),
      simd::msub(z, r.x, simd::mul(x, r.z)),
      simd::msub(x, r.y, simd::mul(y, r.x))
    };
    return out;
  }

  inline void normalize() {
    const auto l   = dot(*this);
    const auto ool = _mm256_rcp_ps(_mm256_sqrt_ps(l));

    x = _mm256_mul_ps(x, ool);
    y = _mm256_mul_ps(y, ool);
    z = _mm256_mul_ps(z, ool);
  }

  inline vector3_t operator-(const vector3_t& r) const {
    return vector3_t(simd::sub(x, r.x), simd::sub(y, r.y), simd::sub(z, r.z));
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
