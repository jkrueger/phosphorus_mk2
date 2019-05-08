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
    const auto zero = simd::load(0.0f);

    const auto gtez_x = gte(ood.x, zero);
    const auto gtez_y = gte(ood.y, zero);
    const auto gtez_z = gte(ood.z, zero);

    auto min_x = select(gtez_x, aabb.max.x, aabb.min.x);
    auto min_y = select(gtez_y, aabb.max.y, aabb.min.y);
    auto min_z = select(gtez_z, aabb.max.z, aabb.min.z);
    auto max_x = select(gtez_x, aabb.min.x, aabb.max.x);
    auto max_y = select(gtez_y, aabb.min.y, aabb.max.y);
    auto max_z = select(gtez_z, aabb.min.z, aabb.max.z);

    min_x = mul(sub(min_x, o.x), ood.x);
    min_y = mul(sub(min_y, o.y), ood.y);
    min_z = mul(sub(min_z, o.z), ood.z);

    max_x = mul(sub(max_x, o.x), ood.x);
    max_y = mul(sub(max_y, o.y), ood.y);
    max_z = mul(sub(max_z, o.z), ood.z);

    const auto n = max(max(min_x, min_y), max(min_z, zero));
    const auto f = min(min(max_x, max_y), min(max_z, d.v));

    const auto mask = lte(n, f);

    dist.v = n;

    return mask;
  }
}
