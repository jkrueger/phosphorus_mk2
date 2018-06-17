#include "linear_bvh_kernel.hpp"
#include "detail/stream.hpp"
#include "accel/bvh.hpp"
#include "accel/triangle.hpp"
#include "math/simd/aabb.hpp"
#include "utils/compiler.hpp"

#include "ImathBoxAlgo.h"

linear_mbvh_kernel_t::linear_mbvh_kernel_t(const accel::mbvh_t* bvh)
  : bvh(bvh)
{}

void linear_mbvh_kernel_t::trace(pipeline_state_t<>* state, active_t<>& active) const {
  for (auto i=0; i<bvh->num_triangles; ++i) {
    const auto& t = bvh->triangles[i];
    t.intersect2(state, active.index, active.num);
  }
}
