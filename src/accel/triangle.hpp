#pragma once

#include "state.hpp"
#include "math/simd.hpp"
#include "math/soa.hpp"

namespace accel {
  /* Members of this namespace store optimized triangle data, and implement 
   * intersection algorithms on them */
  namespace triangle {
    template<int N>
    struct moeller_trumbore_t {
      soa::vector3_t<N> _e0, _e1, _v0;

      uint32_t meshid[N];
      uint32_t setid[N];
      uint32_t faceid[N];

      inline moeller_trumbore_t()
      {
      }

      inline void intersect(
        pipeline_state_t<>& stream
      , uint32_t* indices
      , uint32_t num) const
      {
	// TODO: make a choice whether we iterate over
	// rays or triangles, based on the number of
	// rays and triangles
	
	const auto zero = simd::load(0.0f);
	const auto one  = simd::load(1.0f);
	const auto peps = simd::load(0.00000001f);
	const auto meps = simd::load(-0.00000001f);
	
	auto u = zero;
	auto v = zero;
	auto m = zero;

	const auto rays  = simd::load((int32_t*) indices);
	
	const auto o = simd::vector3_t(
	    stream.rays.p.x
	  , stream.rays.p.y
	  , stream.rays.p.z
	  , rays);
	
	const auto wi = simd::vector3_t(
	    stream.rays.wi.x
	  , stream.rays.wi.y
	  , stream.rays.wi.z
	  , rays);
	
	const auto d =
	  simd::float_t<SIMD_WIDTH>::gather(stream.rays.d, rays);

	const auto t = simd::load((int32_t) 0);
	
	for (auto i=0; i<N; ++i) {
	  const auto e0 = vector3_t<N>(_e0.x[i], _e0.y[i], _e0.z[i]);
	  const auto e1 = vector3_t<N>(_e1.x[i], _e1.y[i], _e1.z[i]);
	  const auto v0 = vector3_t<N>(_v0.x[i], _v0.y[i], _v0.z[i]);

	  const auto p   = cross(wi, e1);
	  const auto det = dot(e0, p);
	  const auto ood = div(one, det);
	  const auto t   = sub(o, v0);
	  const auto q   = cross(t, e0);

	  const auto us  = mul(dot(t, p), ood);
	  const auto vs  = mul(dot(wi, q), ood);

	  auto ds = mul(dot(e1, q), ood);

	  const auto xmask = _or(gt(det, peps), lt(det, meps));
	  const auto umask = gte(us, zero);
	  const auto vmask = _and(gte(vs, zero), lte(add(us, vs), one));
	  const auto dmask = _and(gte(ds, zero), lt(ds, d));

	  const auto mask = _and(_and(_and(vmask, umask), dmask), xmask);

	  u = select(mask, us, u);
	  v = select(mask, vs, v);
	  d = select(mask, ds, d);
	  m = _or(mask, m);

	  t = _select(mask, simd::load(i), t);
	}

	auto mask = simd::movemask(m);

	float ds[N], ts[N];
	
	simd::store(d, ds);
	simd::store(t, ts);

	// TODO: use masked scatter instructions when available
	while(mask != 0) {
	  const auto n = __bscf(mask);
	  if (n < num) {
	    const auto x = *(indices + n);
	    // update shortest hit distance of ray
	    stream.rays.d[x] = ds[ts[n]];
	    // update shading parameters
	    stream.shading.mesh[x] = meshid[ts[n]];
	    stream.shading.set[x]  = setid[ts[n]];
	    stream.shading.face[x] = faceid[ts[n]];
	    stream.shading.u[x]    = u[ts[n]];
	    stream.shading.v[x]    = v[ts[n]];
	  }
	}
      }
    };
  }
}
