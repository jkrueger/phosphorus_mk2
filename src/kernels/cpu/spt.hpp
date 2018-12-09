#pragma once

#include "light.hpp"
#include "state.hpp"
#include "../../bsdf.hpp"
#include "math/vector.hpp"

#include <cmath>

namespace spt {
  /* This stores some state needed by the path integrator */
  template<int N = 1024>
  struct state_t {
    uint16_t depth[N];      // the current depth of the path at an index
    uint16_t path[N];       // the number of paths traced at index
    float pdf[N];           // a pdf for a light sample at an index
    soa::vector3_t<N> beta; // the contribution of the current path vertex
    soa::vector3_t<N> r;    // accumulated radianceover all computed paths

    inline state_t() {
      memset(depth, 0, sizeof(depth));
      memset(path, 0, sizeof(path));
      memset(&r, 0, sizeof(r));

      for (auto i=0; i<N; ++i) {
        beta.from(i, Imath::V3f(1.0f));
      }
    }
  };

  struct light_sampler_t {
    uint32_t paths_per_sample;

    inline light_sampler_t(const parsed_options_t& options)
      : paths_per_sample(options.paths_per_sample)
    {}

    inline void operator()(
      const scene_t& scene
    , sampler_t* sampler
    , const active_t<>& active
    , const interaction_t<>* hits
    , state_t<>* state
    , ray_t<>* samples)
    {
      for (auto i=0; i<active.num; ++i) {
        const auto index = active.index[i];

	if (!hits->is_hit(index)) {
	  continue;
	}

        const auto depth = state->depth[index];
        const auto path  = state->path[index];
	
        // get one light source sample
        const auto light = scene.light(0);

        sampler_t::light_sample_t sample;
        light->sample(sampler->sample2(), sample);

	const auto n = hits->n.at(index);
	const auto p = offset(hits->p.at(index), n);

	auto wi = sample.p - p;

        if (in_same_hemisphere(n, wi)) {
          const auto d = wi.length() - 0.0001f;
          wi.normalize();

          samples->reset(index, p, wi, d);
          samples->shadow(index);

          // the information about the surface we're going to hit
          // comes from the light sample, and not from tracing
          // the ray through the scene, so set everything up here
	  samples->set_surface(
	    index
	  , sample.mesh, sample.set, sample.face
	  , sample.uv.x, sample.uv.y);

	  state->pdf[index] = sample.pdf;
        }
        else {
          samples->mask(index);
        }
      }
    }
  };

  struct integrator_t {
    uint32_t max_depth;
    uint32_t paths_per_sample;

    integrator_t(const parsed_options_t& options)
      : max_depth(options.path_depth)
      , paths_per_sample(options.paths_per_sample)
    {}

    inline void operator()(
      sampler_t* sampler
    , const scene_t& scene
    , active_t<>& active
    , interaction_t<>* primary
    , interaction_t<>* hits
    , state_t<>* state
    , ray_t<>* samples)
    {
      const auto num = active.num;
      active.num = 0;

      for (auto i=0; i<num; ++i) {
	const auto index = active.index[i];

        auto out = state->r.at(index);
        auto keep_alive = true;

	if (hits->is_hit(index)) {
	  if (state->depth[index] == 0 || hits->is_specular(index)) {
	    out += hits->e.at(index);
	  }

	  // compute direct light contribution at the current
	  // path vertex. this gets modulated by the current path weight
	  // and added to the path radiance
	  if (!samples->is_occluded(index)) {
	    out += li(scene, samples, hits, state, index);
          }

          ++state->depth[index];

          // check if we have to terminate the path
          if (terminate_path(sampler, state, index)) {
            assert(state->path[index] < paths_per_sample);
            state->depth[index] = 1;
            state->path[index]++;

            keep_alive = state->path[index] < paths_per_sample;
          }
        }
	else {
	  // TODO: check if we have an environment map

          // revive rays if the primary ray hit anything
          assert(state->path[index] < paths_per_sample);
          state->depth[index] = 1;
          state->path[index]++;

          keep_alive = primary->is_hit(index) &&
            state->path[index] < paths_per_sample;
	}

        if (keep_alive &&
            sample_bsdf(sampler, primary, hits, state, samples, index)) {
          active.add(index);
        }

	state->r.from(index, out);
      }
    }

    Imath::Color3f li(
      const scene_t& scene
    , ray_t<>* samples
    , interaction_t<>* hits
    , state_t<>* state 
    , uint32_t index) const
    {
      const auto bsdf = hits->bsdf[index]; 
      const auto wi   = samples->wi.at(index);
      const auto wo   = hits->wi.at(index);
      const auto n    = hits->n.at(index);

      const auto f = bsdf->f(wi, wo);
      const auto s = f * (std::fabs(n.dot(wi)) / state->pdf[index]);

      const auto mesh = scene.mesh(samples->mesh[index]);
      const auto matid = mesh->material(samples->set[index]);
      const auto material = scene.material(matid);

      shading_result_t light;
      {
        Imath::V3f n;
        Imath::V2f st;

        mesh->shading_parameters(samples, n, st, index);
        material->evaluate(samples->p.at(index), wi, n, st, light);
      }
      
      return state->beta.at(index) * light.e * s;
    }

    bool sample_bsdf(
      sampler_t* sampler
    , const interaction_t<>* primary
    , const interaction_t<>* hits
    , state_t<>* state
    , ray_t<>* rays
    , uint32_t index) const
    {
      bsdf_t* bsdf;
      Imath::V3f p;
      Imath::V3f wi;
      Imath::V3f n;
      Imath::Color3f beta;

      if (state->depth[index] == 1) {
        p  = primary->p.at(index);
        wi = primary->wi.at(index); 
        n  = primary->n.at(index);

        bsdf = primary->bsdf[index];
        beta = Imath::V3f(1.0f);
      }
      else {
        p  = hits->p.at(index);
        wi = hits->wi.at(index);
        n  = hits->n.at(index);

        bsdf = hits->bsdf[index];
        beta = state->beta.at(index);
      }

      assert(bsdf);

      float pdf;
      Imath::V3f sampled;
      Imath::V2f sample = sampler->sample2();

      // sample the bsdf based on the previous path direction
      const auto f = bsdf->sample(sample, wi, sampled, pdf);

      if (color::is_black(f) || pdf == 0.0f) {
        std::cout << "black" << std::endl;
	return false;
      }

      const auto weight = n.dot(sampled);

      state->beta.from(index, beta * (f * (std::fabs(weight) / pdf)));

      rays->reset(index, offset(p, n, weight < 0.0f), sampled);
      rays->specular_bounce(index, bsdf->is_specular());

      return true;
    }

    inline bool terminate_path(
      sampler_t* sampler
    , state_t<>* state
    , uint32_t index) const
    {
      auto w = 1.0f;
      const auto beta = state->beta.at(index);

      bool alive = state->depth[index] < max_depth;
      if (alive) {
        if (state->depth[index] >= 3) {
          float q = std::max((float) 0.05f, 1.0f - color::y(beta));
          alive = sampler->sample() >= q;
          if (alive) {
            w = (1.0f / (1.0f - q));
          }
        }
      }

      state->beta.from(index, beta * w);

      return !alive;
    }
  };
}
