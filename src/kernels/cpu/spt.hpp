#pragma once

#include "state.hpp"

namespace spt {
  struct sampler_t {
    inline void operator()(
      const scene_t& scene
    , pipeline_state_t<>* state
    , active_t<>& active
    , occlusion_query_state_t<>* samples) {
    }
  };

  struct integrator_t {
    inline void operator()(
      pipeline_state_t<>* state
    , active_t<>& active
    , occlusion_query_state_t<>* samples)
    {
      auto hits=0;
      
      for (auto i=0; i<active.num; ++i) {
	const auto index = active.index[i];
	if (state->is_hit(index)) {
	  ++hits;
	  state->shading.r.from(index, Imath::Color3f(1, 1, 1));
	}
	else {
	  state->shading.r.from(index, Imath::Color3f(0, 0, 0));
	}
      }
    }
  };
}
