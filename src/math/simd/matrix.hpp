#pragma once

#include "vector.hpp"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-register"
#include <ImathMatrix.h>
#pragma clang diagnostic pop

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
   * while skipping the 4th column (i.e. the vector will not get traanslated)
   * FIXME: provide a function to actually extract a 3x3 matrix from this matrix */
  inline vector3_t<8> operator* (const vector3_t<8>& v) const {
    vector3_t<8> out;
    for (auto i=0; i<3; ++i) {
      auto x = _mm256_mul_ps(v.v[i], m[i][0]);
      for (auto j=1; j<3; j++) {
	x = _mm256_fmadd_ps(v.v[j], m[i][j], x);
      }
      out.v[i] = x;
    }
    return out;
  }
};

#endif
