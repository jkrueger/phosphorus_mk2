#pragma once

#include "math/simd.hpp"
#include "math/soa.hpp"

namespace accel {
  /* Members of this namespace store optimized triangle data, and implement 
   * intersection algorithms on them */
  namespace triangle {
    template<int N>
    struct moeller_trumbore_t {
      soa::vector3_t<N> e0, e1, v0;

      uint32_t meshid[N];
      uint32_t setid[N];
      uint32_t faceid[N];

      inline moeller_trumbore_t()
      {
      }

      inline typename float_t<N>::type intersect(
        const vector3_t<N>& o
      , const vector3_t<N>& dir
      , const typename float_t<N>::type d) const
      {
	for (auto i=0; i<N; ++i) {
	  const auto e0 = vector3_t<N>(e0.x[N], e0.y[N], e0.z[N]);
	  const auto e1 = vector3_t<N>(e1.x[N], e1.y[N], e1.z[N]);
	  const auto v0 = vector3_t<N>(v0.x[N], v0.y[N], v0.z[N]);

	  const auto p   = cross(dir, e1);
	  const auto det = dot(e0, p);
	  const auto ood = div(one, det);
	  const auto t   = sub(o, v0);
	  const auto q   = cross(t, e0);

	  const auto us  = mul(dot(t, p), ood);
	  const auto vs  = mul(dot(dir, q), ood);

	  auto ds = mul(dot(e1, q), ood);

	  const auto xmask = _or(gt(det, peps), lt(det, meps));
	  const auto umask = gte(us, zero);
	  const auto vmask = _and(gte(vs, zero), lte(add(us, vs), one));
	  const auto dmask = _and(gte(ds, zero), lt(ds, d));

	  return movemask(_and(_and(_and(vmask, umask), dmask), xmask));
	}
      }
    };
  }
}
