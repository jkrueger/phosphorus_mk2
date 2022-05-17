#pragma once

#include "vector.hpp"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-register"
#include <Imath/ImathMatrix.h>
#pragma clang diagnostic pop

namespace simd {
template<int N>
  struct matrix44_t
  {};

#ifdef __SSE__
// TODO: implement 4 wide matrices
#endif

#ifdef __AVX2__

  template<>
  struct matrix44_t<8> {
    __m256 m[4][4];

    inline matrix44_t(const Imath::M44f& m0) {
      for (auto i=0; i<4; ++i) {
        for (auto j=0; j<4; j++) {
         m[i][j] = _mm256_set1_ps(m0.x[i][j]);
        }
      }
    }

    /* transform a 3 dimensional vector, by multiplying it with this matrix
     * while skipping the 4th column (i.e. the vector will not get translated)
     * FIXME: provide a function to actually extract a 3x3 matrix from this matrix */
    inline vector3_t<8> operator* (const vector3_t<8> & v) const {
      vector3_t<8> out;

      auto x = _mm256_mul_ps(v.x, m[0][0]);
      x = _mm256_fmadd_ps(v.y, m[1][0], x);
      x = _mm256_fmadd_ps(v.z, m[2][0], x);
      out.x = x;

      auto y = _mm256_mul_ps(v.x, m[0][1]);
      y = _mm256_fmadd_ps(v.y, m[1][1], y);
      y = _mm256_fmadd_ps(v.z, m[2][1], y);
      out.y = y;

      auto z = _mm256_mul_ps(v.x, m[0][2]);
      z = _mm256_fmadd_ps(v.y, m[1][2], z);
      z = _mm256_fmadd_ps(v.z, m[2][2], z);
      out.z = z;

      return out;
    }
  };

  inline vector3_t<8> transform_vector(
    const matrix44_t<8>& m
  , const vector3_t<8> & v) 
  {
    vector3_t<8> out;

    auto x = _mm256_mul_ps(v.x, m.m[0][0]);
    x = _mm256_fmadd_ps(v.y, m.m[1][0], x);
    x = _mm256_fmadd_ps(v.z, m.m[2][0], x);
    out.x = x;

    auto y = _mm256_mul_ps(v.x, m.m[0][1]);
    y = _mm256_fmadd_ps(v.y, m.m[1][1], y);
    y = _mm256_fmadd_ps(v.z, m.m[2][1], y);
    out.y = y;

    auto z = _mm256_mul_ps(v.x, m.m[0][2]);
    z = _mm256_fmadd_ps(v.y, m.m[1][2], z);
    z = _mm256_fmadd_ps(v.z, m.m[2][2], z);
    out.z = z;

    return out;
  }

  inline vector3_t<8> transform_point(
    const matrix44_t<8>& m
  , const vector3_t<8> & v) 
  {
    vector3_t<8> out;

    auto x = _mm256_mul_ps(v.x, m.m[0][0]);
    x = _mm256_fmadd_ps(v.y, m.m[1][0], x);
    x = _mm256_fmadd_ps(v.z, m.m[2][0], x);
    out.x = _mm256_add_ps(x, m.m[3][0]);

    auto y = _mm256_mul_ps(v.x, m.m[0][1]);
    y = _mm256_fmadd_ps(v.y, m.m[1][1], y);
    y = _mm256_fmadd_ps(v.z, m.m[2][1], y);
    out.y = _mm256_add_ps(y, m.m[3][1]);;

    auto z = _mm256_mul_ps(v.x, m.m[0][2]);
    z = _mm256_fmadd_ps(v.y, m.m[1][2], z);
    z = _mm256_fmadd_ps(v.z, m.m[2][2], z);
    out.z = _mm256_add_ps(z, m.m[3][2]);;

    return out;
  }
}

#endif
