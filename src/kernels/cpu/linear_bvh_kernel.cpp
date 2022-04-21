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

void linear_mbvh_kernel_t::trace(rays_t& rays, active_t& active) const {
  soa::ray_t stream(rays, active);
  for (auto i=0; i<bvh->num_triangles; ++i) {
    const auto& t = bvh->triangles[i];
    t.iterate_rays(stream, rays, active.index, active.num);
  }
}
