#pragma once

#include "float8.hpp"
#include "int8.hpp"
#include "../soa.hpp"

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
      const float_t<8>& _x
    , const float_t<8>& _y
    , const float_t<8>& _z)
    : x(_x.v), y(_y.v), z(_z.v)
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

  template<int M>
  inline vector3_t(const soa::vector3_t<M>& v)
  {
    x = _mm256_load_ps(v.x);
    y = _mm256_load_ps(v.y);
    z = _mm256_load_ps(v.z);
  }

  template<int M>
  inline vector3_t(const soa::vector3_t<M>& v, uint32_t idx)
  {
    x = _mm256_load_ps(v.x + idx);
    y = _mm256_load_ps(v.y + idx);
    z = _mm256_load_ps(v.z + idx);
  }

  template<int M>
  inline void store(soa::vector3_t<M>& out) const {
    _mm256_store_ps(out.x, x);
    _mm256_store_ps(out.y, y);
    _mm256_store_ps(out.z, z);
  }

  template<int M>
  inline void store(soa::vector3_t<M>& out, uint32_t off) const {
    _mm256_store_ps(out.x + off, x);
    _mm256_store_ps(out.y + off, y);
    _mm256_store_ps(out.z + off, z);
  }

  inline vector3_t rcp() const {
    return {_mm256_rcp_ps(x), _mm256_rcp_ps(y), _mm256_rcp_ps(z)};
  }

  inline __m256 dot(const vector3_t& r) const {
    return simd::madd(x, r.x, simd::madd(y, r.y, simd::mul(z, r.z)));
  }

  inline vector3_t cross(const vector3_t& r) const {
    vector3_t out = {
      simd::msub(y, r.z, simd::mul(z, r.y)),
      simd::msub(z, r.x, simd::mul(x, r.z)),
      simd::msub(x, r.y, simd::mul(y, r.x))
    };
    return out;
  }

  inline simd::float_t<8> length2() const {
    return simd::float_t<8>(dot(*this));
  }

  inline simd::float_t<8> length() const {
    return simd::float_t<8>(_mm256_sqrt_ps(length2().v));
  }

  inline void normalize() {
    const auto l   = dot(*this);
    const auto ool = _mm256_rcp_ps(_mm256_sqrt_ps(l));

    x = _mm256_mul_ps(x, ool);
    y = _mm256_mul_ps(y, ool);
    z = _mm256_mul_ps(z, ool);
  }

  inline vector3_t scaled(const float_t<8>& s) const {
    return vector3_t(mul(x, s.v), mul(y, s.v), mul(z, s.v));
  }

  inline vector3_t scaled(const float_t<8>& a, const float_t<8>& b, const float_t<8>& c) const {
    return vector3_t(mul(x, a.v), mul(y, b.v), mul(z, c.v));
  }

  inline vector3_t operator+(const vector3_t& r) const {
    return {simd::add(x, r.x), simd::add(y, r.y), simd::add(z, r.z)};
  }

  inline vector3_t operator-(const vector3_t& r) const {
    return {simd::sub(x, r.x), simd::sub(y, r.y), simd::sub(z, r.z)};
  }

  inline vector3_t operator*(const float_t<8>& r) const {
    return {simd::mul(x, r.v), simd::mul(y, r.v), simd::mul(z, r.v)};
  }
};

  template<int N>
  inline void store(const vector3_t<N>& v, float* x, float* y, float* z);

  template<>
  inline void store<8>(const vector3_t<8>& v, float* x, float* y, float* z) {
    _mm256_store_ps(x, v.x);
    _mm256_store_ps(y, v.y);
    _mm256_store_ps(z, v.z);
  }

  template<int N>
  inline vector3_t<N> offset(
    const vector3_t<N>& p
  , const vector3_t<N>& n
  , bool invert = false)
  {
    const auto off = load(invert ? -0.0001f : 0.0001f);
    return p + n * off;
  }

  template<int N>
  inline int32_t<N> in_same_hemisphere(const vector3_t<N>& a, const vector3_t<N>& b) {
    const auto x = a.dot(b) >= load(0.0f);
    return int32_t<N>(_mm256_castps_si256(x));
  }
}

#endif
