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
 */
namespace spt {
  /* This stores some state needed by the path integrator */
  template<int N = 1024>
  struct state_t {
    const scene_t* scene;
    sampler_t* sampler;

    uint16_t depth[N];      // the current depth of the path at an index
    uint16_t path[N];       // the number of paths traced at index
    float pdf[N];           // a pdf for a light sample at an index
    soa::vector3_t<N> beta; // the contribution of the current path vertex
    soa::vector3_t<N> r;    // accumulated radianceover all computed paths

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
    , ray_t<>* samples) const
    {
      // mark dead paths as alive, so we can generate new shadow rays
      // for these paths
      revive_dead_paths(state, active, primary, hits);

      const auto hit    = simd::int32v_t(HIT);
      const auto masked = simd::int32v_t(MASKED | SHADOW);
      const auto shadow = simd::int32v_t(SHADOW);

      // iterate over alls paths, and generate shadow rays for them
      for (auto i=0; i<active.num; i+=SIMD_WIDTH) {
        // we pick one light for a set of SIMD_WDITH samples. this should
        // be ok, over a large enough number of samples per pixel, and
        // in my opinion doesn't make that much of a difference visually
        // the way samples are layed out at the moment, the SIMD_WIDTH
        // samples will be distributed over SIMD_WIDTH pixels
        const auto nlights = state->scene->num_lights();
        sampler_t::light_samples_t<SIMD_WIDTH> light_samples;

        // no good way to sample lights in parallel atm
        // soa::vector2_t<SIMD_WIDTH> uvs;
        // state->sampler->sample2(uvs);

        // const auto x = state->sampler->sample();
        // const auto l = std::min(std::floor(x * nlights), nlights - 1.0f);

        // const auto light = state->scene->light(l);

        // // generate n samples form the light source
        // light->sample(uvs, light_samples);

        for (auto i=0; i<SIMD_WIDTH; ++i) {
          sampler_t::light_sample_t sample;

          const auto x = state->sampler->sample();
          const auto l = std::min(std::floor(x * nlights), nlights - 1.0f);

          const auto light = state->scene->light(l);

          light->sample(state->sampler->sample2(), sample);

          light_samples.p.x[i] = sample.p.x;
          light_samples.p.y[i] = sample.p.y;
          light_samples.p.z[i] = sample.p.z;

          light_samples.u.v[i] = sample.uv.x;
          light_samples.v.v[i] = sample.uv.y;

          light_samples.mesh[i] = sample.mesh;
          light_samples.face[i] = sample.face;

          light_samples.pdf.v[i] = sample.pdf;
        }

        // the information about the surface we're going to hit
        // comes from the light sample, and not from tracing
        // the ray through the scene, so set everything up here
        samples->set_surface(
          i
        , simd::int32v_t(light_samples.mesh)
        , simd::int32v_t(light_samples.face)
	, light_samples.u
        , light_samples.v);

        // use the sampled light to generate shadow rays
        const simd::vector3v_t n(hits->n, i);
        const auto p = simd::offset(simd::vector3v_t(hits->p, i), n);

	auto wi = light_samples.p - p;

        const auto d = wi.length() - simd::floatv_t(0.0001f);
        wi.normalize();

        // if the light source is behind the sampled point, we mask the
        // shadow ray. this means the result of tracing it through the
        // scene will be ignored
        const auto is_hit = (hit & simd::int32v_t((int32_t*)(hits->flags + i))) == hit;
        const auto ish = simd::in_same_hemisphere(n, wi);
        const auto mask = is_hit & ish;
        const auto flags = simd::select(mask, masked, shadow);

        samples->reset(i, p, wi, d, flags);

        // we store the pdfs for the light samples in the integrator
        // state, since we need them later on
        light_samples.pdf.store(state->pdf + i);
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
    , interaction_t<>* hits
    , ray_t<>* samples) const
    {
      const auto num = active.clear();

      for (auto i=0; i<num; ++i) {
	const auto index = active.index[i];

        auto out = state->r.at(index);
        auto keep_alive = true;

	if (hits->is_hit(i)) {
          if (state->depth[index] == 0 || hits->is_specular(i)) {
            out += state->beta.at(index) * hits->e.at(i);
          }

	  // compute direct light contribution at the current
	  // path vertex. this gets modulated by the current path weight
	  // and added to the path radiance
	  if (!samples->is_occluded(i)) {
	    out += li(state, samples, hits, index, i);
          }

          ++state->depth[index];

          // check if we have to terminate the path
          if (sample_bsdf(state, hits, samples, index, i, active.num)) {
            active.add(index);
          }
          else if (state->path[index] < paths_per_sample) {
            state->mark_for_revive(index);
          }
        }
	else {
          // add environment lighting
          out += state->beta.at(index) * hits->e.at(i);

          if (state->path[index] < paths_per_sample) {
            state->mark_for_revive(index);
          }
	}

	state->r.from(index, out);
      }
    }

    Imath::Color3f li(
      state_t<>* state 
    , ray_t<>* samples
    , interaction_t<>* hits
    , uint32_t index
    , uint32_t to) const
    {
      const auto bsdf = hits->bsdf[to]; 
      const auto wi   = samples->wi.at(to);
      const auto wo   = hits->wi.at(to);
      const auto n    = hits->n.at(to);

      const auto f = bsdf->f(wi, wo);
      const auto s = f * (std::fabs(n.dot(wi)) / state->pdf[to]);

      const auto mesh = state->scene->mesh(samples->meshid(to));
      const auto material = state->scene->material(samples->matid(to));

      assert(material->is_emitter());
 
      shading_result_t light;
      {
        Imath::V3f n;
        Imath::V2f st;
        mesh->shading_parameters(samples, n, st, to);
        material->evaluate(samples->p.at(to), wi, n, st, light);
      }

      return state->beta.at(index) * light.e * s;
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

      // state->depth[index]++;

      if (terminate_path(state, index)) {
        return false;
      }

      p = hits->p.at(from);
      wi = hits->wi.at(from);
      n = hits->n.at(from);
      bsdf = hits->bsdf[from];

      beta = state->beta.at(index);

      assert(bsdf);

      uint32_t flags;
      float pdf;
      Imath::V3f sampled;
      Imath::V2f sample = state->sampler->sample2();

      // sample the bsdf based on the previous path direction
      const auto f = bsdf->sample(sample, wi, sampled, pdf, flags);

      if (color::is_black(f) || pdf == 0.0f) {
        // std::cout << "black" << std::endl;
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
