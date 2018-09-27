#pragma once

#include "state.hpp"

namespace spt {
  struct sampler_t {
    inline void operator()(
      const scene_t& scene
    , pipeline_state_t<>* state
    , active_t<>& active
    , occlusion_query_state_t<>* samples)
    {
      for (auto i=0; i<active.num; ++i) {
        const auto index = active.index[i];
        // get one light source sample
        const auto& sample = sampler.next_light_sample();
        
        const auto wi = sample.p - state->shading.p;
        
        samples->rays.p.from(index, sample.p);
        samples->rays.wi.from(index, wi);

        if (is_same_hemisphere(state.shading.n, wi)) {
          // shadow ray parameters
          samples->params.d[index] = wi.length();
        }
        else {
          samples.mask(i);
        }
      }
    }
  };

  struct integrator_t {
    inline void operator()(
      pipeline_state_t<>* state
    , active_t<>& active
    , occlusion_query_state_t<>* samples)
    {
      for (auto i=0; i<active.num; ++i) {
	const auto index = active.index[i];

	if (state->is_hit(index) && !samples->occluded(i)) {
          // evaluate bsdf and update shading
	}
	else {
          // check if we have an environment map
	}
      }
    }
  };
}
