#pragma once

#include "film.hpp"
#include "sampling.hpp"
#include "jobs/tiles.hpp"
#include "math/simd.hpp"
#include "math/soa.hpp"
#include "utils/compiler.hpp"

/* Global state for the rendering of one frame */
struct frame_state_t {
  sampler_t*    sampler;
  job::tiles_t* tiles;
  film_t<>*     film;

  inline frame_state_t(sampler_t* sampler, job::tiles_t* tiles, film_t<>* film)
    : sampler(sampler)
    , tiles(tiles)
    , film(film)
  {}

  inline ~frame_state_t() {
    delete tiles;
  }
};

namespace details {
  static const uint32_t DEAD   = 0;
  static const uint32_t ALIVE  = 0x1;
  static const uint32_t HIT    = 0x2;
  static const uint32_t MASKED = 0x4;
}

/* The state kept in the pipeline while rendering one 
 * stream of samples, belonign to one tile of the rendered
 * image */
template<int N = 1024>
struct pipeline_state_t {
  static const uint32_t size = N;
  static const uint32_t step = SIMD_WIDTH;

  static const bool stop_on_first_hit = false;

  typedef soa::ray_t<size> ray_t;
  typedef soa::shading_parameters_t<size> shading_parameters_t;
  typedef soa::shading_result_t<size> shading_result_t;

  ray_t                rays;
  shading_parameters_t shading;
  shading_result_t     result;

  soa::vector3_t<size> beta;

  uint8_t flags[size];
  uint8_t depth[size];

  inline pipeline_state_t() {
    memset(flags, 0, size);
    memset(depth, 0, size);
    memset(&result.r, 0, sizeof(result.r));
    memset(&result.e, 0, sizeof(result.e));

    for (auto i=0; i<size; ++i) {
      beta.from(i, Imath::V3f(1.0f));
    }
  }

  inline void miss(uint32_t i) {
    flags[i] &= ~details::HIT;
  }

  inline void hit(uint32_t i, float d) {
    flags[i] |= details::HIT;
    rays.d[i] = d;
  }

  inline bool is_hit(uint32_t i) const {
    return (flags[i] & details::HIT) == details::HIT;
  }

  inline void shade(
    uint32_t i
  , uint32_t mesh, uint32_t set, uint32_t face
  , float u, float v)
  {
    shading.mesh[i] = mesh;
    shading.set[i]  = set;
    shading.face[i] = face;
    shading.u[i]    = u;
    shading.v[i]    = v;
  }
};

/* The state kept while evaluating a stream of occlusion
 * querries  */
template <int N = 1024>
struct occlusion_query_state_t {
  static const uint32_t size = N;
  static const uint32_t step = SIMD_WIDTH;

  static const bool stop_on_first_hit = true;

  typedef soa::ray_t<size> ray_t;
  typedef soa::shading_parameters_t<size> shading_parameters_t;
  typedef soa::shading_result_t<size> shading_result_t;

  ray_t rays;
  shading_parameters_t shading;
  shading_result_t result;

  float   pdf[size];
  uint8_t flags[size];

  inline occlusion_query_state_t() {
    memset(flags, 0, sizeof(flags));
  }

  inline void mask(uint32_t i) {
    flags[i] |= details::MASKED;
  }

  inline void miss(uint32_t i) {
    flags[i] &= ~details::HIT;
  }

  inline void hit(uint32_t i, float d) {
    flags[i] |= details::HIT;
    rays.d[i] = d;
  }

  inline bool is_hit(uint32_t i) const {
    return (flags[i] & details::HIT) == details::HIT;
  }

  inline bool is_masked(uint32_t i) const {
    return (flags[i] & details::MASKED) == details::MASKED;
  } 

  inline bool is_occluded(uint32_t i) const {
    return is_hit(i) || is_masked(i);
  }

  inline void shade(
    uint32_t i
  , uint32_t mesh, uint32_t set, uint32_t face
  , float u, float v)
  {
    shading.mesh[i] = mesh;
    shading.set[i]  = set;
    shading.face[i] = face;
    shading.u[i]    = u;
    shading.v[i]    = v;
  }
};

/* active masks for the pipeline and occlusion query states, 
   to track which rays are currently active */
template<int N = 1024>
struct active_t {
  uint32_t num;
  uint32_t index[N];

  inline active_t()
    : num(0)
  {}

  inline void reset(uint32_t base) {
    num = N;
    for (auto i=0; i<N; ++i) {
      index[i] = base + i;
    }
  }

  inline void add(uint32_t i) {
    index[num++] = i;
  }

  inline bool has_live_paths() const {
    return num > 0;
  }
};
