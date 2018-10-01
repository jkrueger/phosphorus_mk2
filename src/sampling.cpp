#include "sampling.hpp"
#include "light.hpp"
#include "scene.hpp"

#include <cmath>
#include <random> 
#include <functional> 

struct sampler_t::details_t {
  std::mt19937gen;
};

sampler_t::sampler_t()
  : details(new details_t())
{
  std::uniform_real_distribution<float> dis(0.0f,1.0f);
  
  for (auto i=0; i<128; ++i) {
    for (auto j=0; j<8; ++j) {
      pixel_samples.v[i].x[j] = dis(details->gen) - 0.5f;
      pixel_samples.v[i].y[j] = dis(details->gen) - 0.5f;
    }
  }
}

sampler_t::~sampler_t() {
  delete details;
}

void sampler_t::preprocess(const scene_t& scene) {
  std::uniform_real_distribution<float> dis(0.0f,1.0f);

  for (auto i=0; i<scene.num_lights(); ++i) {
    const auto light = scene.light(i);

    light->sample({dis(gen), dis(gen)}, light_samples[i]);
  }
}
