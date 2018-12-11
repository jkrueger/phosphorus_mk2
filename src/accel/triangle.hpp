#pragma once

#include "state.hpp"
#include "../mesh.hpp"
#include "../triangle.hpp"
#include "math/simd.hpp"
#include "math/soa.hpp"
#include "utils/nocopy.hpp"

#include <cmath> 

namespace accel {
  /* Members of this namespace store optimized triangle data, and implement 
   * intersection algorithms on them */
  namespace triangle {
    template<int N>
    struct moeller_trumbore_t /* : nocopy_t */ {
      soa::vector3_t<N> _e0, _e1, _v0;

      uint32_t meshid[N];
      uint32_t setid[N];
      uint32_t faceid[N];

      uint32_t num;

      inline moeller_trumbore_t(const triangle_t** triangles, uint32_t num)
	: num(num)
      {
	for (auto i=0; i<num; ++i) {
	  const auto e0 = triangles[i]->b() - triangles[i]->a();
	  const auto e1 = triangles[i]->c() - triangles[i]->a();
	  const auto v0 = triangles[i]->a();

	  _e0.x[i] = e0.x; _e0.y[i] = e0.y; _e0.z[i] = e0.z;
	  _e1.x[i] = e1.x; _e1.y[i] = e1.y; _e1.z[i] = e1.z;
	  _v0.x[i] = v0.x; _v0.y[i] = v0.y; _v0.z[i] = v0.z;

	  meshid[i] = triangles[i]->mesh->id;
	  setid[i] = triangles[i]->set;
	  faceid[i] = triangles[i]->face;
	}
      }

      /** 
       * Intersect each ray with each triangle stored in this accelerator, one by one
       * This is mostly here as a baseline algorithm to verify simd implementations 
       */
      template<typename T>
      inline void baseline(
        T* stream
      , uint32_t* indices
      , uint32_t num_rays) const {
        for (auto i=0; i<num_rays; ++i) {
          for (auto j=0; j<num; ++j) {
            const auto index = indices[i];

            auto v0v1 = _e0.at(j);
            auto v0v2 = _e1.at(j);
            auto p    = stream->wi.at(index).cross(v0v2);

            auto det = v0v1.dot(p);

            if (std::abs(det) < 0.00000001) {
              continue;
            }

            auto ood = 1.0 / det;

            auto t = stream->p.at(index) - _v0.at(j);

            auto u = t.dot(p) * ood;
            if (u < 0.0 || u > 1.0) {
              continue;
            }

            auto q = t.cross(v0v1);

            auto v = stream->wi.at(index).dot(q) * ood;
            if (v < 0.0 || (u + v) > 1.0) {
              continue;
            }
            
            auto d = v0v2.dot(q) * ood;
            if (d < 0.0 || d > stream->d[index]) {
              continue;
            }

            if (!stream->is_shadow(index)) {
              stream->shade(index, meshid[j], setid[j], faceid[j], u, v);
            }
	    stream->hit(index, d);
          }
        }
      }

      template<typename T>
      inline void iterate_rays(
	T* stream
      , uint32_t* indices
      , uint32_t num_rays) const
      {
	const auto
	  one  = simd::load(1.0f),
	  zero = simd::load(0.0f),
	  peps = simd::load(0.00000001f),
	  meps = simd::load(-0.00000001f);

	vector3_t<N> e0(_e0.x, _e0.y, _e0.z);
        vector3_t<N> e1(_e1.x, _e1.y, _e1.z);
	vector3_t<N> v0(_v0.x, _v0.y, _v0.z);

	for (auto i=0; i<num_rays; ++i) {
	  const auto index = indices[i];

	  vector3_t<N> o(
	      stream->p.x[index]
	    , stream->p.y[index]
	    , stream->p.z[index]);

	  vector3_t<N> wi(
	      stream->wi.x[index]
	    , stream->wi.y[index]
	    , stream->wi.z[index]);

	  const auto d = simd::load(stream->d[index]);

	  const auto p   = wi.cross(e1);
	  const auto det = e0.dot(p);
	  const auto ood = simd::div(one, det);
	  const auto t   = o - v0;
	  const auto q   = t.cross(e0);

	  const auto us = simd::mul(t.dot(p), ood);
	  const auto vs = simd::mul(wi.dot(q), ood);
	  const auto ds = simd::mul(e1.dot(q), ood);

	  const auto xmask = simd::_or(simd::gt(det, peps), simd::lt(det, meps));
	  const auto umask = simd::gte(us, zero);
	  const auto vmask = simd::_and(simd::gte(vs, zero), simd::lte(simd::add(us, vs), one));

	  const auto dmask = simd::_and(simd::gte(ds, zero), simd::lt(ds, d));

	  auto mask =
	    simd::movemask(
	      simd::_and(
                simd::_and(
		  simd::_and(vmask, umask),
		  dmask),
		xmask));

	  if (mask != 0) {
	    float dists[N];
	    float closest = stream->d[index];

	    simd::store(ds, dists);

	    int idx = -1;
	    while(mask != 0) {
	      auto x = __bscf(mask);
	      if (dists[x] < closest && x < num) {
		closest = dists[x];
		idx = x;
	      }
	    }

	    if (idx != -1) {
	      float u[N];
	      float v[N];

	      simd::store(us, u);
	      simd::store(vs, v);

              if (!stream->is_shadow(index)) {
                stream->set_surface(
                  index
                , meshid[idx]
                , setid[idx]
                , faceid[idx]
                , u[idx]
                , v[idx]);
              }
	      stream->hit(index, closest);
	    }
	  }
	}
      }

      template<typename Stream>
      inline void iterate_triangles(
        Stream* stream
      , uint32_t* indices
      , uint32_t num_rays) const
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

	const auto rays = simd::load((int32_t*) indices);

	const auto o = simd::vector3_t(
          simd::float_t<SIMD_WIDTH>::gather(stream->p.x, rays)
        , simd::float_t<SIMD_WIDTH>::gather(stream->p.y, rays)
        , simd::float_t<SIMD_WIDTH>::gather(stream->p.z, rays));
   
	const auto wi = simd::vector3_t(
          simd::float_t<SIMD_WIDTH>::gather(stream->wi.x, rays)
        , simd::float_t<SIMD_WIDTH>::gather(stream->wi.y, rays)
        , simd::float_t<SIMD_WIDTH>::gather(stream->wi.z, rays));
	
	auto d = simd::float_t<SIMD_WIDTH>::gather(stream->d, rays);
	auto j = simd::load((int32_t) -1);
	
	for (auto i=0; i<num; ++i) {
	  const auto e0 = vector3_t<N>(_e0.x[i], _e0.y[i], _e0.z[i]);
	  const auto e1 = vector3_t<N>(_e1.x[i], _e1.y[i], _e1.z[i]);
	  const auto v0 = vector3_t<N>(_v0.x[i], _v0.y[i], _v0.z[i]);

	  const auto p   = wi.cross(e1);
	  const auto det = e0.dot(p);
	  const auto ood = simd::div(one, det);
	  const auto t   = o - v0;
	  const auto q   = t.cross(e0);

	  const auto us = simd::mul(t.dot(p), ood);
	  const auto vs = simd::mul(wi.dot(q), ood);
	  const auto ds = simd::mul(e1.dot(q), ood);

	  const auto xmask = simd::_or(simd::gt(det, peps), simd::lt(det, meps));
	  const auto umask = simd::gte(us, zero);
	  const auto vmask = simd::_and(simd::gte(vs, zero), simd::lte(simd::add(us, vs), one));
	  const auto dmask = simd::_and(simd::gte(ds, zero), simd::lt(ds, d));

	  const auto mask =
	    simd::_and(
	      simd::_and(
	        simd::_and(vmask, umask),
		dmask),
	      xmask);

	  u = simd::select(mask, u, us);
	  v = simd::select(mask, v, vs);
	  d = simd::select(mask, d, ds);
	  m = simd::_or(mask, m);

	  j = simd::select(mask, j, simd::load(i));
	}

	auto mask = simd::movemask(m);

	float   ds[N];
	int32_t ts[N];
	
	simd::store(d, ds);
	simd::store(j, ts);

	// TODO: use masked scatter instructions when available
	while(mask != 0) {
	  const auto r = __bscf(mask);
	  if (r < num_rays) {
	    const auto x = indices[r];
	    const auto t = ts[r];

            if (!stream->is_shadow(x)) {
              stream->set_surface(
                x
              , meshid[t]
              , setid[t]
              , faceid[t]
              , u[r]
              , v[r]);
            }
	    stream->hit(x, ds[r]);
	  }
	}
      }
    };
  }
}
