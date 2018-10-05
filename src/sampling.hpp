#pragma once

#include "math/simd.hpp"
#include "utils/compiler.hpp"

struct scene_t;

struct sampler_t {
  struct details_t* details;

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

  struct light_sample_t {
    Imath::V3f p;
    Imath::V2f uv;
    uint32_t material;
    float pdf;
  };

  typedef pixel_sample_t<> pixel_samples_t;

  pixel_samples_t pixel_samples;
  light_sample_t* light_samples;

  sampler_t();
  ~sampler_t();

  void preprocess(const scene_t& scene);

  inline const pixel_samples_t& next_pixel_samples() const {
    return pixel_samples;
  }

  inline const light_sample_t& next_light_sample() const {
    return light_samples[0];
  }
};
