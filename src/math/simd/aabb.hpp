#pragma once

#include "float8.hpp"
#include "vector.hpp"

namespace simd {
  template<int N>
  struct aabb_t {
    vector3_t<N> min, max;

    inline aabb_t(const float* const bounds)
      : min(&(bounds[0*N]), &(bounds[1*N]), &(bounds[2*N]))
      , max(&(bounds[3*N]), &(bounds[4*N]), &(bounds[5*N]))
    {}
  };

  template<int N>
  inline float_t<N> intersect(
    const aabb_t<N>& aabb
  , const vector3_t<N>& o
  , const vector3_t<N>& ood
  , const float_t<N>& d
  , float_t<N>& dist);

  template<>
  inline float_t<8> intersect<8>(
    const aabb_t<8>& aabb
  , const vector3_t<8>& o
  , const vector3_t<8>& ood
  , const float_t<8>& d
  , float_t<8>& dist)
  {
    const auto zero = float_t<8>(0.0f);

    const auto ax = (aabb.min.x - o.x) * ood.x;
    const auto ay = (aabb.min.y - o.y) * ood.y;
    const auto az = (aabb.min.z - o.z) * ood.z;

    const auto bx = (aabb.max.x - o.x) * ood.x;
    const auto by = (aabb.max.y - o.y) * ood.y;
    const auto bz = (aabb.max.z - o.z) * ood.z;

    const auto min_x = min(ax, bx);
    const auto min_y = min(ay, by);
    const auto min_z = min(az, bz);

    const auto max_x = max(ax, bx);
    const auto max_y = max(ay, by);
    const auto max_z = max(az, bz);

    const auto n = max(max(min_x, min_y), max(min_z, zero.v));
    const auto f = min(min(max_x, max_y), min(max_z, d.v));

    const auto mask = lte(n, f);

    dist.v = n;

    return mask;
  }
}
