#pragma once

#include "state.hpp"

namespace accel {
  struct mbvh_t;
}

/**
 * Iterates all leafs of the tree and tests against all triangles.
 * This exists only for debugging purposes
 *
 */
struct linear_mbvh_kernel_t {
  const accel::mbvh_t* bvh;

  linear_mbvh_kernel_t(const accel::mbvh_t* bvh);

  /* find the closest intersection point for all rays in the
   * current work item in the pipeline */
  void trace(ray_t<>* rays, active_t<>& active) const;

  inline void operator()(ray_t<>* rays, active_t<>& active) const {
    trace(rays, active);
  }
};
