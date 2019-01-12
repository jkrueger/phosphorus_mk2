#pragma once

#include "state.hpp"
#include "utils/allocator.hpp"

namespace accel {
  struct mbvh_t;
}

struct stream_mbvh_kernel_t {
  struct state_t;

  const accel::mbvh_t* bvh;

  stream_mbvh_kernel_t(const accel::mbvh_t* bvh);

  /* find the closest intersection point for all rays in the
   * current work item in the pipeline */
  void trace(state_t* state, ray_t<>* rays, active_t<>& active) const;

  inline void operator()(state_t* state, ray_t<>* rays, active_t<>& active) const {
    trace(state, rays, active);
  }

  static state_t* make_state();

  static void destroy_state(state_t* state);
};
