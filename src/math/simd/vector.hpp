#pragma once

#include "float8.hpp"
#include "int8.hpp"

#include <ImathVec.h>

#include <xmmintrin.h>

namespace simd {
  
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

  __forceinline vector3_t()
  {}

  __forceinline vector3_t(const vector3_t& cpy)
    : x(cpy.x), y(cpy.y), z(cpy.z)
  {}

  __forceinline vector3_t(
      const __m256& _x
    , const __m256& _y
    , const __m256& _z)
    : x(_x), y(_y), z(_z)
  {}

  __forceinline vector3_t(
      const float_t<8>& _x
    , const float_t<8>& _y
    , const float_t<8>& _z)
    : x(_x.v), y(_y.v), z(_z.v)
  {}

  __forceinline vector3_t(
    const float _x
  , const float _y
  , const float _z)
  {
    x = _mm256_set1_ps(_x);
    y = _mm256_set1_ps(_y);
    z = _mm256_set1_ps(_z);
  }

  __forceinline vector3_t(const Imath::V3f& v)
    : vector3_t(v.x, v.y, v.z)
  {}

  __forceinline vector3_t(
    const float* _x
  , const float* _y
  , const float* _z)
  {
    x = _mm256_load_ps(_x);
    y = _mm256_load_ps(_y);
    z = _mm256_load_ps(_z);
  }

  /* loads a vector from an SOA data structure via gather instructions */
  __forceinline vector3_t(
    const float* _x
  , const float* _y
  , const float* _z
  , const int32_t<8>& idx)
  {
    x = _mm256_i32gather_ps(_x, idx.v, 4);
    y = _mm256_i32gather_ps(_y, idx.v, 4);
    z = _mm256_i32gather_ps(_z, idx.v, 4);
  }

  __forceinline void store(float* _x, float* _y, float* _z) const {
    _mm256_store_ps(_x, x);
    _mm256_store_ps(_y, y);
    _mm256_store_ps(_z, z);
  }

  __forceinline vector3_t rcp() const {
    return {_mm256_rcp_ps(x), _mm256_rcp_ps(y), _mm256_rcp_ps(z)};
  }

  __forceinline float_t<8> dot(const vector3_t& r) const {
    return simd::madd(x, r.x, simd::madd(y, r.y, simd::mul(z, r.z)));
  }

  __forceinline vector3_t cross(const vector3_t& r) const {
    vector3_t out = {
      simd::msub(y, r.z, simd::mul(z, r.y)),
      simd::msub(z, r.x, simd::mul(x, r.z)),
      simd::msub(x, r.y, simd::mul(y, r.x))
    };
    return out;
  }

  __forceinline simd::float_t<8> length2() const {
    return dot(*this);
  }

  __forceinline simd::float_t<8> length() const {
    return simd::float_t<8>(_mm256_sqrt_ps(length2().v));
  }

  __forceinline void normalize() {
    const auto l   = dot(*this);
    const auto ool = _mm256_rcp_ps(_mm256_sqrt_ps(l.v));

    x = _mm256_mul_ps(x, ool);
    y = _mm256_mul_ps(y, ool);
    z = _mm256_mul_ps(z, ool);
  }

  __forceinline vector3_t scaled(const float_t<8>& s) const {
    return vector3_t(mul(x, s.v), mul(y, s.v), mul(z, s.v));
  }

  __forceinline vector3_t scaled(const float_t<8>& a, const float_t<8>& b, const float_t<8>& c) const {
    return vector3_t(mul(x, a.v), mul(y, b.v), mul(z, c.v));
  }

  __forceinline vector3_t operator+(const vector3_t& r) const {
    return {simd::add(x, r.x), simd::add(y, r.y), simd::add(z, r.z)};
  }

  __forceinline vector3_t operator-(const vector3_t& r) const {
    return {simd::sub(x, r.x), simd::sub(y, r.y), simd::sub(z, r.z)};
  }

  __forceinline vector3_t operator*(const float_t<8>& r) const {
    return {simd::mul(x, r.v), simd::mul(y, r.v), simd::mul(z, r.v)};
  }
};

  template<int N>
  __forceinline vector3_t<N> offset(
    const vector3_t<N>& p
  , const vector3_t<N>& n
  , bool invert = false)
  {
    const auto off = load(invert ? -0.0001f : 0.0001f);
    return p + n * off;
  }

  template<int N>
  __forceinline int32_t<N> in_same_hemisphere(const vector3_t<N>& a, const vector3_t<N>& b) {
    const auto x = a.dot(b) >= float_t<N>(0.0f);
    return int32_t<N>(_mm256_castps_si256(x.v));
  }
}

#endif
