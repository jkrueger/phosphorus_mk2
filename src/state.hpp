#pragma once

#include "sampling.hpp"
#include "jobs/tiles.hpp"
#include "math/simd.hpp"
#include "math/soa.hpp"
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

/* The state kept in the pipeline while rendering one 
 * stream of samples, belonign to one tile of the rendered
 * image */
template<int N = 1024>
struct pipeline_state_t {
  static const uint32_t DEAD  = 0;
  static const uint32_t ALIVE = 0x1;
  static const uint32_t HIT   = 0x2;

  static const uint32_t size = N;
  static const uint32_t step = SIMD_WIDTH;

  typedef soa::ray_t<step> ray_t;

  ray_t rays[size/step];

  struct {
    // ...
  } shading[size/step];

  uint8_t flags[size];

  inline void miss(uint32_t i) {
    flags[i] &= (~(HIT << 16));
  }

  inline void hit(uint32_t i) {
    flags[i] |= (HIT << 16);
  } 
};

/* The state kept while evaluating a stream of occlusion
 * querries  */
template <int N = 1024>
struct occlusion_query_state_t {
  static const uint32_t size = N;
  static const uint32_t step = SIMD_WIDTH;

  typedef soa::ray_t<step> ray_t;

  ray_t rays[size/step];

};

/* active masks for the pipeline and occlusion query states, 
   to track which rays are currently active */
template<int N = 1024>
struct active_t {
  uint32_t num;
  uint32_t index[N];
};
