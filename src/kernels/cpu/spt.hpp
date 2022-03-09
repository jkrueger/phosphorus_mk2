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
  /* This stores some state needed by the path integrator */
  template<int N = 1024>
  struct state_t {
    const scene_t* scene;
    sampler_t* sampler;

    uint16_t depth[N]; // the current depth of the path at an index
    uint16_t path[N];  // the number of paths traced at index

    // light samples for the current integrator iteration
    sampler_t::light_sample_t light_samples[N];

    soa::vector3_t<N> beta; // the contribution of the current path vertex
    soa::vector3_t<N> r;    // accumulated radiance over all computed paths

    active_t<> dead;

    inline state_t(const scene_t* scene, sampler_t* sampler)
      : scene(scene), sampler(sampler)
    {}

    inline void reset() {
      memset(depth, 0, sizeof(depth));
      memset(path, 0, sizeof(path));
      memset(&r, 0, sizeof(r));

      for (auto i=0; i<N; ++i) {
        beta.from(i, Imath::V3f(1.0f));
      }

      dead.clear();
    }

    inline decltype(auto) next_light_samples() {
      return sampler->next_light_samples();
    }

    inline decltype(auto) fresh_light_samples() {
      return sampler->fresh_light_samples(scene, light_samples, N);
    }

    inline void mark_for_revive(uint32_t i) {
      dead.add(i);
    }

    inline void revive(uint32_t i) {
      depth[i] = 0;
      path[i]++;
      beta.from(i, Imath::V3f(1.0f));
    }
  };

  struct light_sampler_t {
    uint32_t paths_per_sample;

    inline light_sampler_t(const parsed_options_t& options)
      : paths_per_sample(options.paths_per_sample)
    {}

    inline void revive_dead_paths(
      state_t<>* state
    , active_t<>& active 
    , const interaction_t<>* primary
    , interaction_t<>* hits) const
    {
      assert(active.num + state->dead.num <= active_t<>::size);

      for (auto i=0; i<state->dead.num; ++i) {
        const auto pixel = state->dead.index[i];
        const auto index = active.num;

        if (primary->is_hit(pixel)) {
          hits->from(primary, pixel, index);
          state->revive(pixel);
          active.add(pixel);
        }
      }

      state->dead.clear();
    }

    inline void operator()(
      state_t<>* state
    , active_t<>& active
    , const interaction_t<>* primary
    , interaction_t<>* hits
    , ray_t<>* rays) const
    {
      // mark dead paths as alive, so we can generate new shadow rays
      // for these paths
      revive_dead_paths(state, active, primary, hits);

      // get fresh light samples and store them in the integrator state
      const auto samples = state->fresh_light_samples();

      for (auto i=0; i<active.num; ++i) {
        const auto n = hits->n.at(i);
        const auto p = offset(hits->p.at(i), n);

        auto wi = samples->light->setup_shadow_ray(p, samples[i]);

        const auto d  = wi.length() - 0.0001f;

        wi.normalize();

        rays->reset(i, p, wi, d);

        if (hits->is_hit(i) && in_same_hemisphere(n, wi)) {
          rays->shadow(i);
        }
        else {
          rays->mask(i);
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
      state_t<>* state
    , active_t<>& active
    , interaction_t<>* primary
    , interaction_t<>* hits
    , ray_t<>* samples) const
    {
      const auto num = active.clear();

      for (auto i=0; i<num; ++i) {
        const auto index = active.index[i];

        auto out = state->r.at(index);

        if (hits->is_hit(i)) {
          // add direct lighting to path vertex
          if (state->depth[index] == 0 || hits->is_specular(i)) {
            out += state->beta.at(index) * hits->e.at(i);
          }

          // compute direct light contribution at the current
          // path vertex. this gets modulated by the current path weight
          // and added to the path radiance
          if (!samples->is_occluded(i)) {
  	        out += state->beta.at(index) * li(state, samples, hits, i);
          }

          ++state->depth[index];

          // check if we have to terminate the path
          if (sample_bsdf(state, hits, samples, index, i, active.num)) {
            active.add(index);
          }

          // else if ((state->path[index]+1) < paths_per_sample) {
          //   state->mark_for_revive(index);
          // }
        }
        else {
          // add environment lighting
          out += state->beta.at(index) * hits->e.at(i);

          // if ((state->path[index]+1) < paths_per_sample) {
          //   state->mark_for_revive(index);
          // }
        }

        state->r.from(index, out);
      }
    }

    Imath::Color3f li(
      state_t<>* state 
    , ray_t<>* samples
    , interaction_t<>* hits
    , uint32_t to) const
    {
      assert(state && samples && hits);

      const auto bsdf = hits->bsdf[to]; 
      const auto p    = samples->p.at(to);
      const auto wi   = samples->wi.at(to);
      const auto wo   = hits->wi.at(to);
      const auto n    = hits->n.at(to);
      const auto d    = samples->d[to];

      assert(bsdf);

      // should never happen
      if (!bsdf) {
        throw std::runtime_error("NO BSDF!");
        return Imath::Color3f(0.0f);
      }

      const auto& sample = state->light_samples[to];

      const auto f = bsdf->f(wi, wo);
      const auto e = sample.light->le(*state->scene, sample, wi);

      const auto pdf = sample.pdf; // * d * d;

      return (e * 4) * f * (1.0f / pdf);
    }

    bool sample_bsdf(
      state_t<>* state
    , const interaction_t<>* hits
    , ray_t<>* rays
    , uint32_t index
    , uint32_t from 
    , uint32_t to) const
    {
      bsdf_t* bsdf;
      Imath::V3f p;
      Imath::V3f wi;
      Imath::V3f n;
      Imath::Color3f beta;

      if (terminate_path(state, index)) {
        return false;
      }

      p = hits->p.at(from);
      wi = hits->wi.at(from);
      n = hits->n.at(from);
      bsdf = hits->bsdf[from];

      beta = state->beta.at(index);
      if (!bsdf) {
        return false;
      }

      uint32_t flags;
      float pdf;
      Imath::V3f sampled;
      Imath::V2f sample = state->sampler->sample2();

      // sample the bsdf based on the previous path direction
      const auto f = bsdf->sample(sample, wi, sampled, pdf, flags);

      if (color::is_black(f) || pdf == 0.0f) {
        return false;
      }

      const auto weight = n.dot(sampled);

      state->beta.from(index, beta * (f * (std::fabs(weight) / pdf)));

      rays->reset(to, offset(p, n, weight < 0.0f), sampled);
      rays->specular_bounce(to, bsdf_t::is_specular(flags));

      return true;
    }

    inline bool terminate_path(
      state_t<>* state
    , uint32_t index) const
    {
      auto w = 1.0f;
      const auto beta = state->beta.at(index);

      bool alive = state->depth[index] < max_depth;
      if (alive) {
        if (state->depth[index] >= 3) {
          float q = std::max((float) 0.05f, 1.0f - color::y(beta));
          alive = state->sampler->sample() >= q;
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
