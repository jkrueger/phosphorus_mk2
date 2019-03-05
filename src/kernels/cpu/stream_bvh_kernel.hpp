#pragma once

#include "state.hpp"
#include "utils/allocator.hpp"

namespace accel {
  struct mbvh_t;
}

struct stream_mbvh_kernel_t {
  struct details_t;
  details_t* details;

  const accel::mbvh_t* bvh;

  stream_mbvh_kernel_t(const accel::mbvh_t* bvh);
  ~stream_mbvh_kernel_t();

  /* find the closest intersection point for all rays in the
   * current work item in the pipeline */
  void trace(ray_t<>* rays, active_t<>& active) const;

  inline void operator()(ray_t<>* rays, active_t<>& active) const {
    trace(rays, active);
  }
};
