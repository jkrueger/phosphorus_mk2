#pragma once

#include "sampling.hpp"
#include "jobs/tiles.hpp"
#include "math/simd.hpp"
#include "utils/compiler.hpp"

/* Global state for the rendering of one frame */
struct frame_state_t {
  sampler_t     sampler;
  job::tiles_t* tiles;

  inline frame_state_t(job::tiles_t* tiles)
    : tiles(tiles)
  {}

  inline ~frame_state_t() {
    delete tiles;
  }
};

namespace detail {
  template<int N>
  struct alignas(32) vector_t {
    float x[N];
    float y[N];
    float z[N];
  };

  /* a stream of rays */
  template<int N>
  struct ray_t {
    vector_t<N> p;
    vector_t<N> wi;
    float d[N];
  };
}

/* The state kept in the pipeline while rendering one 
 * stream of samples, belonign to one tile of the rendered
 * image */
template<int N = 1024>
struct pipeline_state_t {
  static const uint32_t size = N;
  static const uint32_t step = SIMD_WIDTH;

  typedef detail::ray_t<size> ray_t;

  ray_t rays[size];
};

/* The state kept while evaluating a stream of occlusion
 * querries  */
template <int N = 1024>
struct occlusion_query_state_t {
  detail::ray_t<8> ray[N];
};
