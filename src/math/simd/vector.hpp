#pragma once

#include "float8.hpp"
#include "int8.hpp"

#include <Imath/ImathVec.h>

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
      x = _mm256_loadu_ps(_x);
      y = _mm256_loadu_ps(_y);
      z = _mm256_loadu_ps(_z);
    }

    /* loads a vector from an SOA data structure via gather instructions */
    inline vector3_t(
      const float* _x
    , const float* _y
    , const float* _z
    , const int32_t<8>& idx)
    {
      x = _mm256_i32gather_ps(_x, idx.v, 4);
      y = _mm256_i32gather_ps(_y, idx.v, 4);
      z = _mm256_i32gather_ps(_z, idx.v, 4);
    }

    inline void store(float* _x, float* _y, float* _z) const {
      _mm256_store_ps(_x, x);
      _mm256_store_ps(_y, y);
      _mm256_store_ps(_z, z);
    }

    inline vector3_t rcp() const {
      return {_mm256_rcp_ps(x), _mm256_rcp_ps(y), _mm256_rcp_ps(z)};
    }

    inline float_t<8> dot(const vector3_t& r) const {
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
      return dot(*this);
    }

    inline simd::float_t<8> length() const {
      return simd::sqrt(length2());
    }

    inline vector3_t normal() {
      const auto l   = dot(*this);
      const auto ool = _mm256_rcp_ps(_mm256_sqrt_ps(l.v));

      return vector3_t(_mm256_mul_ps(x, ool), _mm256_mul_ps(y, ool), _mm256_mul_ps(z, ool));
    }

    inline void normalize() {
      const auto l   = dot(*this);
      const auto ool = simd::rcp(simd::sqrt(l));

      x = _mm256_mul_ps(x, ool.v);
      y = _mm256_mul_ps(y, ool.v);
      z = _mm256_mul_ps(z, ool.v);
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
  struct vector2_t {
    float_t<8> x, y;

    inline vector2_t()
      : x(0.0f), y(0.0f)
    {}

    inline vector2_t(const vector2_t& cpy) 
    {}

    inline vector2_t(
        const float_t<8>& x
      , const float_t<8>& y)
      : x(x), y(y)
    {}

    inline vector2_t(
      const float x
    , const float y)
      : x(x), y(y)
    {}

    inline vector2_t(const Imath::V2f& v)
      : vector2_t(v.x, v.y)
    {}

    inline vector2_t(
      const float* _x
    , const float* _y)
    {
      x = _mm256_load_ps(_x);
      y = _mm256_load_ps(_y);
    }

    inline vector2_t scaled(const float_t<N>& s) const {
      return vector2_t(x * s, y * s);
    }
  };

  template<int N>
  inline vector2_t<N> operator+(const vector2_t<N>& l, const vector2_t<N>& r) {
    return { l.x + r.x, l.y + r.y };
  }

  template<int N>
  inline vector2_t<N> operator-(const vector2_t<N>& l, const vector2_t<N>& r) {
    return { l.x - r.x, l.y - r.y };
  }

  template<int N>
  inline vector2_t<N> operator*(const vector2_t<N>& l, const float_t<N>& r) {
    return { l.x * r, l.y * r };
  }

  template<int N>
  inline vector2_t<N> operator*(const float_t<N>& l, const vector2_t<N>& r) {
    return { l * r.x, l * r.y };
  }

  template<int N>
  inline vector2_t<N> operator/(const vector2_t<N>& l, const float_t<8>& r) {
    return { l.x / r, l.y / r };
  }

#endif

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
    const auto x = a.dot(b) >= float_t<N>(0.0f);
    return int32_t<N>(_mm256_castps_si256(x.v));
  }
}
