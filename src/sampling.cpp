#include "sampling.hpp"
#include "light.hpp"
#include "scene.hpp"
#include "math/sampling.hpp"

#include <cmath>
#include <random> 
#include <functional> 

struct sampler_t::details_t {
  std::mt19937 gen;
  std::uniform_real_distribution<float> dis;

  inline details_t()
    : dis(0.0f, 1.0f)
  {}

  inline float sample() {
    return dis(gen);
  }

  inline Imath::V2f sample2() {
    return { sample(), sample() };
  }
};

sampler_t::sampler_t(parsed_options_t& options)
  : details(new details_t())
  , spp(options.samples_per_pixel)
  , paths_per_sample(options.paths_per_sample)
  , path_depth(options.path_depth)
{}

sampler_t::~sampler_t() {
  delete pixel_samples;
  delete light_samples;
  delete details;
}

void sampler_t::preprocess(const scene_t& scene) {
  pixel_samples = new pixel_sample_t<>[spp];

  auto spd = (uint32_t) std::sqrt(spp);

  Imath::V2f stratified[spp];
  sample::stratified_2d([this]() { return details->sample(); }, stratified, spd);

  for (auto i=0; i<spp; ++i) {
    for (auto j=0; j<128; ++j) {
      for (auto k=0; k<8; ++k) {
	pixel_samples[i].v[j].x[k] = stratified[i].x;
	pixel_samples[i].v[j].y[k] = stratified[i].y;
      }
    }
  }

  light_samples = new light_sample_t[path_depth*spp*paths_per_sample];

  // auto spd = (uint32_t) std::sqrt(spp);

  // Imath::V2f stratified[spp];
  // sample::stratified_2d([this]() { return details->sample(); }, stratified, spd);

  for (auto i=0; i<spp; i++) {
    for (auto j=0; j<paths_per_sample; ++j) {
      for (auto k=0; k<path_depth; ++k) {
        // TODO: allow sampling of an arbitrary numer of light sources
        const auto light = scene.light(0);
        const auto index = i*paths_per_sample+j;
        light->sample(details->sample2(), light_samples[index*path_depth+k]);
      }
    }
  }
}

float sampler_t::sample() {
  return details->sample();
}

Imath::V2f sampler_t::sample2() {
  return details->sample2();
}
