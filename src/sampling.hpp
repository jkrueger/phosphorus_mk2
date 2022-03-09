#pragma once

#include "options.hpp"
#include "math/config.hpp"
#include "math/sampling.hpp"
#include "math/soa.hpp"
#include "math/simd.hpp"
#include "utils/assert.hpp"
#include "utils/compiler.hpp"

struct scene_t;
struct light_t;

namespace sampling {
  namespace details {
    template<int N>
    struct pixel_samples_t {
      static const uint32_t size=N;
      static const uint32_t step=SIMD_WIDTH;

      soa::vector2_t<step> film[size/step];
      soa::vector2_t<step> lens[size/step];
    };

    struct light_sample_t {
      // the light this sample was taken from
      const light_t* light;

      // sample point information
      Imath::V3f p;
      Imath::V2f uv;
      float pdf;
      float area;

      // light type based information needed for light
      // evaluation
      uint64_t data;
    };

    template<int N>
    struct light_samples_t {
      static const uint32_t size=N;
      static const uint32_t step=SIMD_WIDTH;
      
      struct {
        light_t* light[step];

        soa::vector3_t<step> p;
        float u[step];
        float v[step];
        float pdf[step];

        uint64_t data[step];
      } samples[size/step];
    };
  }
}

struct sampler_t {
  struct details_t;
  details_t* details;

  typedef sampling::details::pixel_samples_t<
    config::STREAM_SIZE
    > pixel_samples_t;
  typedef sampling::details::light_samples_t<
    config::STREAM_SIZE
    > light_samples_t;

  typedef sampling::details::light_sample_t light_sample_t;

  // typedef soa::vector2_t<config::STREAM_SIZE> samples2d_t;

  pixel_samples_t* pixel_samples;
  light_samples_t* light_samples;

  const uint32_t spp;

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

  /** create a precomputed set of 1d samples */
  // uint32_t request_1d_samples();

  /** create a precomputed set of 2d samples */
  // uint32_t request_2d_samples();
  
  inline const pixel_samples_t& next_pixel_samples(uint32_t i) const {
    assert(i < spp);
    return pixel_samples[i];
  }

  const light_samples_t& next_light_samples();

  light_sample_t* fresh_light_samples(const scene_t* scene, light_sample_t* out, size_t n);

  // const float* next_1d_samples(uint32_t id);

  // const samples2d_t& next_2d_samples(uint32_t id);
};
