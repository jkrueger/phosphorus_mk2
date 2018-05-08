#pragma once

#include "state.hpp"

namespace accel {
  struct mbvh_t;
}

struct stream_mbvh_kernel_t {
  const accel::mbvh_t* bvh;

  stream_mbvh_kernel_t(const accel::mbvh_t* bvh);

  /* find the closest intersection point for all rays in the
   * current work item in the pipeline */
  void trace(pipeline_state_t<>& state, active_t<>& active) const;

  inline void operator()(pipeline_state_t<>& state, active_t<>& active) const {
    trace(state, active);
  }
};
