#pragma once

#include "simd/int8.hpp"
#include "simd/float8.hpp"
#include "simd/matrix.hpp"
#include "simd/vector.hpp"

namespace simd {
#ifdef __AVX2__
#define SIMD_WIDTH 8
#endif

  typedef vector3_t<SIMD_WIDTH> vector3_t;
  typedef matrix44_t<SIMD_WIDTH> matrix44_t;
}
