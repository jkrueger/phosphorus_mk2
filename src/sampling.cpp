#include "sampling.hpp"
#include "light.hpp"
#include "scene.hpp"

#include <cmath>
#include <random> 
#include <functional> 

struct sampler_t::details_t {
  std::mt19937 gen;
  std::uniform_real_distribution<float> dis;

  inline details_t()
    : dis(0.0f, 1.0f)
  {}
};

sampler_t::sampler_t(parsed_options_t& options)
  : details(new details_t())
  , spp(options.samples_per_pixel)
{
  pixel_samples = new pixel_sample_t<>[options.samples_per_pixel];

  for (auto i=0; i<options.samples_per_pixel; ++i) {
    for (auto j=0; j<128; ++j) {
      for (auto k=0; k<8; ++k) {
	pixel_samples[i].v[j].x[k] = details->dis(details->gen) - 0.5f;
	pixel_samples[i].v[j].y[k] = details->dis(details->gen) - 0.5f;
      }
    }
  }
}

sampler_t::~sampler_t() {
  delete pixel_samples;
  delete light_samples;
  delete details;
}

void sampler_t::preprocess(const scene_t& scene) {
  light_samples = new light_sample_t[9];

  for (auto i=0; i<9; ++i) {
    const auto light = scene.light(0);
    const auto u = details->dis(details->gen);
    const auto v = details->dis(details->gen);

    light->sample({u, v}, light_samples[i]);
  }
}

Imath::V2f sampler_t::sample2() {
  return { details->dis(details->gen), details->dis(details->gen) };
}
