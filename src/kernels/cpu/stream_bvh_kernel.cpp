#include "stream_bvh_kernel.hpp"
#include "detail/stream.hpp"
#include "accel/bvh.hpp"
#include "accel/triangle.hpp"
#include "math/simd/aabb.hpp"
#include "utils/compiler.hpp"

#include "ImathBoxAlgo.h"

struct stream_mbvh_kernel_t::details_t{
  stream::lanes_t<accel::mbvh_t::width> lanes;
  stream::task_t tasks[256];
};

namespace details {
  template<typename T>
  inline void swap(T& a, T& b) {
    a = a ^ b;
    b = a ^ b;
    a = a ^ b;
  }

  template<int N>
  inline uint32_t compress_and_sort_by_distance(
    size_t mask
  , const int32_t* num_rays
  , const float* dists
  , uint32_t* ids) {
    const auto n = _mm_popcnt_u64(mask);

    if (n < 5) {
      // sort using sorting networks
      auto a = __bscf(mask);
      if (mask == 0) {
        ids[0] = a;
      }
      else {
        auto b = __bscf(mask);
        if (mask == 0) {
          if (dists[b] < dists[a]) { swap(a, b); }

          ids[0] = a;
          ids[1] = b;
        }
        else {
          auto c = __bscf(mask);
          if (dists[b] < dists[a]) { swap(a, b); }
          if (dists[c] < dists[b]) { swap(c, b); }
          if (dists[b] < dists[a]) { swap(b, a); }

          if (mask == 0) {
            ids[0] = a;
            ids[1] = b;
            ids[2] = c;
          }
          else {
            auto d = __bscf(mask);

            if (dists[b] < dists[a]) { swap(a, b); }
            if (dists[d] < dists[c]) { swap(d, c); }
            if (dists[c] < dists[a]) { swap(c, a); }
            if (dists[d] < dists[b]) { swap(d, b); }
            if (dists[c] < dists[b]) { swap(c, b); }

            ids[0] = a;
            ids[1] = b;
            ids[2] = c;
            ids[3] = d;
          }
        }
      }
    }
    else {
      // fallback insertion sort
      auto m = 0;
      for (auto i=0; i<N; ++i) {
        auto num = num_rays[i];
        if (num > 0) {
          auto d = dists[i];
          ids[m] = i;
          for(auto j=m; j>0; --j) {
            if (d < dists[ids[j]]) {
              swap(ids[j], ids[j-1]);
            }
          }
          ++m;
        }
      }
    }

    return n;
  }
}

/* Implements MBVH-RS algorithm for tracing a set of rays through 
 * the scene */
template<typename Stream>
void intersect(
  stream_mbvh_kernel_t::details_t* state
, Stream* stream
, const active_t<>& active
, const accel::mbvh_t* bvh)
{
  typedef simd::float_t<accel::mbvh_t::width> float_t;

  auto& lanes = state->lanes;
  auto& tasks = state->tasks;

  // set initial lane for root node, including all rays
  lanes.init(active, stream);

  // all rays were masked
  if (lanes.num[0] == 0) {
    return;
  }

  const float_t zero(0.0f);
  const simd::int32v_t one(1);

  auto top = 0;
  push(tasks, top, lanes.num[0]);

  while (top > 0) {
    auto& cur = tasks[--top];

    if (!cur.is_leaf()) {
      const auto& node = bvh->root[cur.offset];
      auto todo = pop(lanes, cur.lane, cur.num_rays);

      __aligned(64) const simd::aabb_t<accel::mbvh_t::width> bounds(node.bounds);

      simd::int32v_t num_active(0);

      auto length = zero;
      auto end    = todo + cur.num_rays;
      while (todo != end) {
	float_t dist;

	const auto ray = *todo;

	if (stream->is_shadow(ray) && stream->is_hit(ray)) {
	   ++todo;
	   continue;
	}

	auto hits =
	  simd::intersect<accel::mbvh_t::width>(
	    bounds
	  , stream->p.v_at(ray)
	  , stream->wi.v_at(ray).rcp()
          , stream->d[ray]
	  , dist);

        num_active = num_active + (one & hits);
	length = length + (dist & hits);

	auto mask = simd::to_mask(hits);

	// push ray into lanes for intersected nodes
	while(mask != 0) {
	  auto x = __bscf(mask);
	  push(lanes, x, *todo);
	}

	++todo;
      }

      uint32_t ids[8];

      auto lane_mask = simd::to_mask(num_active != simd::int32v_t(0));

      __aligned(32) int32_t num_rays[8];
      num_active.store(num_rays);

      __aligned(32) float dists[8];
      length.store(dists);
	
      auto n = details::compress_and_sort_by_distance<
        accel::mbvh_t::width
        >(lane_mask, num_rays, dists, ids);

      for (auto i=0; i<n; ++i) {
	push(tasks, top, node, ids[i], num_rays[ids[i]]);
      }
    }
    else {
      auto todo  = pop(lanes, cur.lane, cur.num_rays);
      auto begin = todo; 
      auto end   = todo + cur.num_rays;
      while (begin < end) {
	auto index = cur.offset;
	auto prims = 0;
	const auto num = std::min(end - begin, (long) accel::mbvh_t::width);

	do {
          if (num < 8) {
	    bvh->triangles[index].iterate_rays(stream, begin, num);
	  }
	  else {
	    bvh->triangles[index].iterate_triangles(stream, begin, num);
	  }
	  prims += accel::mbvh_t::width;
	  ++index;
	} while(unlikely(prims < cur.prims));

	begin+=accel::mbvh_t::width;
      }
    }
  }
}

stream_mbvh_kernel_t::stream_mbvh_kernel_t(const accel::mbvh_t* bvh)
  : details(new details_t())
  , bvh(bvh)
{}

stream_mbvh_kernel_t::~stream_mbvh_kernel_t() {
  delete details;
}

void stream_mbvh_kernel_t::trace(ray_t<>* rays, active_t<>& active) const {
  intersect(details, rays, active, bvh);
}
