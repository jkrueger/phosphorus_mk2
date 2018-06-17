#include "sampling.hpp"

#include <cmath>
#include <random> 
#include <functional> 

sampler_t::sampler_t() {
  std::mt19937 gen;
  std::uniform_real_distribution<float> dis(0.0f,1.0f);
  
  for (auto i=0; i<128; ++i) {
    for (auto j=0; j<8; ++j) {
      pixel_samples.v[i].x[j] = dis(gen) - 0.5f;
      pixel_samples.v[i].y[j] = dis(gen) - 0.5f;
    }
  }
}
