#pragma once

#include "state.hpp"
#include "../../bsdf.hpp"
#include "math/orthogonal_base.hpp"
#include "math/vector.hpp"

#include <cmath>

namespace spt {
  struct light_sampler_t {
    inline void operator()(
      sampler_t* sampler
    , pipeline_state_t<>* state
    , active_t<>& active
    , occlusion_query_state_t<>* samples)
    {
      for (auto i=0; i<active.num; ++i) {
        const auto index = active.index[i];
	if (!state->is_hit(index)) {
	  continue;
	}
	
        // get one light source sample
        const auto& sample = sampler->next_light_sample(state->depth[index]);

	const auto n = state->shading.n.at(index);
	const auto p = offset(state->rays.p.at(index), n);

	auto wi = sample.p - p;

        if (in_same_hemisphere(n, wi)) {
	  samples->rays.p.from(index, p);
	  samples->rays.d[index] = wi.length() - 0.0001f;

	  wi.normalize();
	  samples->rays.wi.from(index, wi);

	  samples->shade(
	    index
	  , sample.mesh, sample.set, sample.face
	  , sample.uv.x, sample.uv.y);

	  samples->pdf[index] = sample.pdf;
        }
        else {
          samples->mask(index);
        }
      }
    }
  };

  struct integrator_t {

    static const uint32_t DEFAULT_PATH_DEPTH = 3;

    uint32_t max_depth;

    integrator_t()
      : max_depth(DEFAULT_PATH_DEPTH)
    {}

    inline void operator()(
      sampler_t* sampler
    , const scene_t& scene
    , pipeline_state_t<>* state
    , active_t<>& active
    , occlusion_query_state_t<>* samples)
    {
      const auto num = active.num;
      active.num = 0;

      for (auto i=0; i<num; ++i) {
	const auto index = active.index[i];

	auto out = state->result.r.at(index) + state->result.e.at(index);
	
	if (state->is_hit(index)) {
	  const auto bsdf = state->result.bsdf[index];
	
	  const auto wi = samples->rays.wi.at(index);
	  const auto wo = -state->rays.wi.at(index);
	  const auto n  = state->shading.n.at(index);

	  // compute direct light contribution at the current
	  // path vertex. this gets modulated by the current path weight
	  // and added to the path radiance
	  if(!samples->is_occluded(index)) {
	    out += li(scene, bsdf, n, wi, wo, state, samples, index);
	  }

	  // update state with new path segments based on sampling
	  // the bsdf at the current path vertex
	  sample_bsdf(sampler, bsdf, n, wo, state, index);

	  // check if we have to terminate the path
	  if (!terminate_path(state->depth[index])) {
	    ++state->depth[index];
	    active.add(index);
	  }
	}
	else {
	  // TODO: check if we have an environment map
	}

	state->result.r.from(index, out);
      }
    }

    Imath::Color3f li(
      const scene_t& scene
    , const bsdf_t* bsdf
    , const Imath::V3f& n
    , const Imath::V3f& wi
    , const Imath::V3f& wo
    , pipeline_state_t<>* state
    , occlusion_query_state_t<>* samples
    , uint32_t index) const
    {
      const auto f = bsdf->f(wi, wo);
      const auto s = std::fabs(n.dot(wi)) / samples->pdf[index];

      const auto mesh = scene.mesh(samples->shading.mesh[index]);
      const auto matid = mesh->material(samples->shading.set[index]);
      const auto material = scene.material(matid);

      mesh->shading_parameters(samples->shading, index);
      material->evaluate(scene, samples, index);

      return state->beta.at(index) * (f * samples->result.e.at(index) * s);
    }

    void sample_bsdf(
      sampler_t* sampler
    , const bsdf_t* bsdf
    , const Imath::V3f& n
    , const Imath::V3f& wi
    , pipeline_state_t<>* state
    , uint32_t index) const
    {
      float pdf;
      Imath::V3f sampled;
      Imath::V2f sample = sampler->sample2();

      const orthogonal_base_t base(n);

      // sample the bsdf based on the previous path direction
      const auto f = bsdf->sample(sample, wi, sampled, pdf);

      // transform the sampled direction back to world
      // state->rays.wo = state->wi;
      const auto weight = n.dot(wi);
      const auto beta   = state->beta.at(index);

      state->rays.wi.from(index, base.to_world(sampled));
      state->beta.from(index, beta * (f * (std::fabs(weight) / pdf)));
      state->rays.p.from(index, offset(state->rays.p.at(index), n, weight < 0.0f));
    }

    inline bool terminate_path(uint8_t depth) const {
      return depth >= max_depth /*|| (depth >= 3 && terminate_ray(beta))*/;
    }

    // inline bool terminate_path(color_t& beta) const {
    //   float_t q = std::max((float_t) 0.05f, 1.0f - beta.y());
    //   if (dis(gen) < q) {
    // 	return true;
    //   }
    //   beta *= (1.0f / (1.0f - q));
    //   return false;
    // }
  };
}
