#include "sampling.hpp"

#include <cmath>
#include <random> 
#include <functional> 

namespace rng {
  static thread_local std::mt19937 gen;
  static thread_local std::uniform_real_distribution<float> dis(0.0f,1.0f);
}

sampler_t::sampler_t() {
  for (auto i=0; i<128; ++i) {
    for (auto j=0; j<8; ++j) {
      pixel_samples.v[i].x[j] = rng::dis(rng::gen);
      pixel_samples.v[i].y[j] = rng::dis(rng::gen);
    }
  }
}
