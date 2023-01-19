#pragma once

#include "simd/int8.hpp"
#include "simd/float8.hpp"
#include "simd/matrix.hpp"
#include "simd/vector.hpp"

//#ifdef __AVX512__
//#define SIMD_WIDTH 16
#if defined(__AVX2__) 
#define SIMD_WIDTH 8
#endif

namespace simd {
  typedef float_t<SIMD_WIDTH> floatv_t;
  typedef int32_t<SIMD_WIDTH> int32v_t;

  typedef vector2_t<SIMD_WIDTH> vector2v_t;
  typedef vector3_t<SIMD_WIDTH> vector3v_t;
  typedef matrix44_t<SIMD_WIDTH> matrix44v_t;
}
