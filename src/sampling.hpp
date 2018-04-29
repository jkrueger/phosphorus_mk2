#pragma once

struct sampler_t {

  template<int N=8>
  struct vector2_t {
    float x[N];
    float y[N];
  };

  template<int N=1024>
  struct pixel_sample_t {
    vector2_t<8> v[N];
  };

  typedef pixel_sample_t<> pixel_samples_t;

  pixel_samples_t pixel_samples;

  sampler_t();

  inline const pixel_samples_t& next_pixel_samples() {
    return pixel_samples;
  }
};
