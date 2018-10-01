#pragma once

#include "state.hpp"
#include "math/vector.hpp"

namespace spt {
  struct sampler_t {
    inline void operator()(
      pipeline_state_t<>* state
    , active_t<>& active
    , occlusion_query_state_t<>* samples)
    {
      for (auto i=0; i<active.num; ++i) {
        const auto index = active.index[i];
        // get one light source sample
        const auto& sample = sampler.next_light_sample();

	const auto n  = state.shading.n.at(index);
        const auto wi = sample.p - state->shading.p.at(index);

        if (is_same_hemisphere(n, wi)) {
	  samples->params.d[index] = wi.length();
	  samples->rays.p.from(index, offset(sample.p, n));

	  wi.normalize();
	  samples->rays.wi.from(index, wi);
	  samples->params.pdf[index] = sample.pdf;
	  samples->params.material = sample.material;
        }
        else {
          samples.mask(i);
        }
      }
    }
  };

  struct integrator_t {
    inline void operator()(
      const scene_t& scene
    , pipeline_state_t<>* state
    , active_t<>& active
    , occlusion_query_state_t<>* samples)
    {
      for (auto i=0; i<active.num; ++i) {
	const auto index = active.index[i];

	if (state->is_hit(index) && !samples->is_occluded(index)) {
	  const auto il = samples.rays.wi.at(index);
	  const auto ol = -state->shading.rays.wi.at(index);

	  const auto r = state->shading.bsdf->f(il, ol);
	  const auto s = dot(state.shading.n.at(index), ol) /
	    samples->params.pdf[index];

	  const auto m = scene.material(samples->params.material[index]);
	  material->evaluate(scene, samples, );

	  state->result.r.from((r * samples->result.e.at(index)) * s);
	}
	else {
          // check if we have an environment map
	}
      }
    }
  };
}
