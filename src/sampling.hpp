#pragma once

#include "math/simd.hpp"
#include "utils/compiler.hpp"

struct sampler_t {

  template<int N=8>
  struct alignas(32) vector2_t {
    float x[N];
    float y[N];
  };

  template<int N=1024>
  struct pixel_sample_t {
    static const uint32_t size=N;
    static const uint32_t step=SIMD_WIDTH;

    vector2_t<step> v[size/step];
  };

  typedef pixel_sample_t<> pixel_samples_t;

  pixel_samples_t pixel_samples;

  sampler_t();

  inline const pixel_samples_t& next_pixel_samples() {
    return pixel_samples;
  }
};
