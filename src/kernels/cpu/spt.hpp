#pragma once

#include "light.hpp"
#include "state.hpp"
#include "../../bsdf.hpp"
#include "math/vector.hpp"

#include <cmath>

/**
 * Path tracing integrator
 *
 * This integrator implemtns path tracing over a path over samples
 * in parallel. All paths are uni directional starting at the camera
 *
 * FIXME: path revival is broken at the moment. the color of the first
 * intersection is not kept, and so additional paths will darken the image.
 * this code could probably use a complete rewrite
 */
namespace spt {
  /* Stores values that describe the state of a single path in the 
   * pipeline */
  struct path_state_t {
    uint16_t depth; // the current depth of the path at an index
    uint16_t path;  // the number of paths traced at index

    Imath::V3f beta; // the contribution of the current path vertex
    Imath::V3f r;    // accumulated radiance over all computed paths

    inline void reset() {
      depth = 0;
      beta = Imath::V3f(1.0f);
      r = Imath::V3f(0.0f);
    }

    inline void decend() {
      ++depth;
    }
  };

  /* This stores some state needed by the path integrator */
  struct state_t {
    static const uint32_t size = config::STREAM_SIZE;

    const scene_t* scene;
    sampler_t* sampler;

    // states of all paths in the pipeline
    path_state_t path[size];

    // the number of paths traced at index
    uint16_t num_paths[size];

    // light samples for the current integrator iteration
    sampler_t::light_sample_t light_samples[size];

    // currently active paths
    active_t active;
    // paths that got terminated, and can be reused to
    // sample a new path
    active_t dead;

    inline state_t(const scene_t* scene, sampler_t* sampler)
      : scene(scene), sampler(sampler)
    {}

    inline void reset() {
      for (auto i=0; i<size; ++i) {
        path[i].reset();
        num_paths[i] = 0;
      }

      active.reset();
      dead.clear();
    }

    inline decltype(auto) next_light_samples() {
      return sampler->next_light_samples();
    }

    inline decltype(auto) fresh_light_samples() {
      return sampler->fresh_light_samples(scene, light_samples, size);
    }

    inline bool has_live_paths() const {
      return active.has_active();
    }

    inline void mark_for_revive(uint32_t i) {
      dead.add(i);
    }

    inline void revive(uint32_t i) {
      ++num_paths[i];
      path[i].depth = 0;
      path[i].beta = Imath::V3f(1.0f);
    }

    inline Imath::V3f r(uint32_t k) const {
      return path[k].r;
    }
  };

  struct light_sampler_t {
    uint32_t paths_per_sample;

    inline light_sampler_t(const parsed_options_t& options)
      : paths_per_sample(options.paths_per_sample)
    {}

    inline void revive_dead_paths(
      state_t* state
    , active_t& active 
    , const interactions_t& primary
    , interactions_t& hits) const
    {
      assert(active.num + state->dead.num <= active_t::size);

      for (auto i=0; i<state->dead.num; ++i) {
        const auto pixel = state->dead.index[i];
        const auto index = active.num;

        if (primary[pixel].is_hit()) {
          // hits[pixel].from(primary, index);
          state->revive(pixel);
          active.add(pixel);
        }
      }

      state->dead.clear();
    }

    inline void operator()(
      state_t* state
    , const interactions_t& primary
    , interactions_t& hits
    , rays_t& rays) const
    {
      auto& active = state->active;

      // mark dead paths as alive, so we can generate new shadow rays
      // for these paths
      revive_dead_paths(state, active, primary, hits);

      // get fresh light samples and store them in the integrator state
      const auto samples = state->fresh_light_samples();

      for (auto i=0; i<active.num; ++i) {
        const interaction_t& hit = hits[i];
        ray_t& ray = rays[i];

        const auto p = offset(hit.p, hit.n);

        auto wi = samples->light->setup_shadow_ray(p, samples[i]);
        const auto d  = wi.length() - 0.0001f;
        wi.normalize();

        ray.reset(
          p, wi, 
          hit.is_hit() && in_same_hemisphere(hit.n, wi) ? 
            SHADOW : MASKED, 
          d);
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
      state_t* state
    , const interactions_t& primary
    , const interactions_t& hits
    , rays_t& samples) const
    {
      auto& active = state->active;
      const auto num = active.clear();

      for (auto i=0; i<num; ++i) {
        const auto index = active.index[i];

        auto& path = state->path[index];

        const auto& hit  = hits[i];
        const auto& from = samples[i];

        auto& to = samples[active.num];
        const auto& light_sample = state->light_samples[active.num];

        auto out = path.r;

        if (hit.is_hit()) {
          // add direct lighting to path vertex
          if (/* path.depth == 0 || */ hit.is_specular()) {
            out += path.beta * hit.e;
          }

          // compute direct light contribution at the current
          // path vertex. this gets modulated by the current path weight
          // and added to the path radiance
          if (!from.is_occluded()) {
  	        out += path.beta * li(state, from, hit, light_sample);
          }

          ++path.depth;

          // check if we have to terminate the path
          if (sample_bsdf(state, hit, to, index)) {
            active.add(index);
          }

          // else if ((state->path[index]+1) < paths_per_sample) {
          //   state->mark_for_revive(index);
          // }
        }
        else {
          // add environment lighting
          // out += path.beta * hit.e;

          // if ((state->path[index]+1) < paths_per_sample) {
          //   state->mark_for_revive(index);
          // }
        }

        path.r = out;
      }
    }

    Imath::Color3f li(
      state_t* state 
    , const ray_t& ray
    , const interaction_t& hit
    , const sampling::details::light_sample_t& sample) const
    {
      // should never happen
      if (!hit.bsdf) {
        std::cout << "Hitpoint doesn't have a BSDF" << std::endl;
        return Imath::Color3f(0.0f);
      }

      const auto f = hit.bsdf->f(ray.wi, hit.wi);
      const auto e = sample.light->le(*state->scene, sample, ray.wi);

      const auto pdf = sample.pdf; // * d * d;

      return (e * 4) * f * (1.0f / pdf);
    }

    bool sample_bsdf(
      state_t* state
    , const interaction_t& hit
    , ray_t& ray
    , uint32_t index) const
    {
      Imath::Color3f beta;

      if (terminate_path(state, index)) {
        return false;
      }

      beta = state->path[index].beta;
      if (!hit.bsdf) {
        return false;
      }

      uint32_t flags;
      float pdf;
      Imath::V3f sampled;
      Imath::V2f sample = state->sampler->sample2();

      // sample the bsdf based on the previous path direction
      const auto f = hit.bsdf->sample(sample, hit.wi, sampled, pdf, flags);

      if (color::is_black(f) || pdf == 0.0f) {
        return false;
      }

      const auto weight = hit.n.dot(sampled);

      state->path[index].beta = beta * (f * (std::fabs(weight) / pdf));

      ray.reset(offset(hit.p, hit.n, weight < 0.0f), sampled);

      ray.diffuse_bounce(bsdf_t::is_diffuse(flags));
      ray.specular_bounce(bsdf_t::is_specular(flags));
      ray.glossy_bounce(bsdf_t::is_glossy(flags));

      return true;
    }

    inline bool terminate_path(
      state_t* state
    , uint32_t index) const
    {
      auto w = 1.0f;
      auto& beta  = state->path[index].beta;
      auto& depth = state->path[index].depth;

      bool alive = depth < max_depth;
      if (alive) {
        if (depth >= 3) {
          float q = std::max((float) 0.05f, 1.0f - color::y(beta));
          alive = state->sampler->sample() >= q;
          if (alive) {
            w = (1.0f / (1.0f - q));
          }
        }
      }

      beta = beta * w;

      return !alive;
    }
  };
}
