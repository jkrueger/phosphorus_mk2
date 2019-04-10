#pragma once

#include "options.hpp"
#include "math/soa.hpp"
#include "math/simd.hpp"
#include "utils/assert.hpp"
#include "utils/compiler.hpp"

struct scene_t;

struct sampler_t {
  struct details_t;
  details_t* details;

  template<int N=1024>
  struct pixel_sample_t {
    static const uint32_t size=N;
    static const uint32_t step=SIMD_WIDTH;

    soa::vector2_t<step> v[size/step];
  };

  struct light_sample_t {
    Imath::V3f p;
    Imath::V2f uv;
    uint32_t mesh;
    uint32_t face;
    float pdf;
  };

  template<int N>
  struct light_samples_t {
    simd::vector3_t<N> p;
    simd::float_t<N> u;
    simd::float_t<N> v;
    simd::float_t<N> pdf;
    int32_t mesh[N];
    int32_t face[N];
  };

  typedef pixel_sample_t<> pixel_samples_t;

  pixel_samples_t* pixel_samples;
  light_sample_t*  light_samples;

  const uint32_t spp;
  const uint32_t paths_per_sample;
  const uint32_t path_depth;

  sampler_t(parsed_options_t& options);
  ~sampler_t();

  void preprocess(const scene_t& scene);

  float sample();
  
  Imath::V2f sample2();

  template<int N>
  inline void sample2(soa::vector2_t<N>& out) {
    for (auto i=0; i<N; ++i) {
      const auto sample = sample2();
      out.from(i, sample);
    }
  }

  inline const pixel_samples_t& next_pixel_samples(uint32_t i) const {
    assert(i < spp);
    return pixel_samples[i];
  }

  inline const light_sample_t& next_light_sample(uint32_t s, uint32_t i) const {
    assert(s < spp*paths_per_sample);
    assert(i < path_depth);
    return light_samples[s*path_depth+i];
  }
};
