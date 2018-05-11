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

  typedef soa::ray_t<size>     ray_t;
  typedef soa::shading_t<size> shading_t;

  ray_t     rays;
  shading_t shading;

  uint8_t flags[size];

  inline pipeline_state_t() {
    memset(flags, 0, sizeof(flags));
  }

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

  typedef soa::ray_t<size> ray_t;

  ray_t rays;

};

/* active masks for the pipeline and occlusion query states, 
   to track which rays are currently active */
template<int N = 1024>
struct active_t {
  uint32_t num;
  uint32_t index[N];

  inline active_t() {
    reset(0);
  }

  inline void reset(uint32_t base) {
    num = N;
    for (auto i=0; i<N; ++i) {
      index[i] = i;
    }
  }
};
