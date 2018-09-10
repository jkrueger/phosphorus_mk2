#include "stream_bvh_kernel.hpp"
#include "detail/stream.hpp"
#include "accel/bvh.hpp"
#include "accel/triangle.hpp"
#include "math/simd/aabb.hpp"
#include "utils/compiler.hpp"

#include "ImathBoxAlgo.h"

/* Implements MBVH-RS algorithm for tracing a set of rays through 
 * the scene */
template<typename Stream>
void intersect(Stream* stream, const active_t<>& active, const accel::mbvh_t* bvh) {
  static thread_local stream::lanes_t<accel::mbvh_t::width> lanes;
  static thread_local stream::task_t tasks[256];

  // set initial lane for root node, including all rays
  lanes.init(active);

  // all rays were masked
  if (lanes.num[0] == 0) {
    return;
  }

  auto zero = simd::load(0.0f);

  auto top = 0;
  push(tasks, top, lanes.num[0]);

  while (top > 0) {
    auto& cur = tasks[--top];

    if (!cur.is_leaf()) {
      const auto& node = bvh->root[cur.offset];
      auto todo = pop(lanes, cur.lane, cur.num_rays);

      uint32_t num_active[accel::mbvh_t::width] = {[0 ... 7] = 0};

      __aligned(64) const simd::aabb_t<accel::mbvh_t::width> bounds(node.bounds);

      auto length = zero;
      auto end    = todo + cur.num_rays;
      while (todo != end) {
	// if (Stream::stop_on_first_hit && ray.segment->is_hit()) {
	//   ++todo;
	//   continue;
	// }

	typename simd::float_t<accel::mbvh_t::width>::type dist;

	const auto  ray  = *todo;
	const auto& rays = stream->rays;

	auto hits =
	  simd::intersect<accel::mbvh_t::width>(
	    bounds
	  , simd::vector3_t(rays.p.x[ray], rays.p.y[ray], rays.p.z[ray])
	  , simd::vector3_t(1.0f/rays.wi.x[ray], 1.0f/rays.wi.y[ray], 1.0f/rays.wi.z[ray])
	  , simd::load(rays.d[ray])
	  , dist);

	length = simd::add(length, simd::_and(dist, hits));

	auto mask = simd::movemask(hits);

	// push ray into lanes for intersected nodes
	while(mask != 0) {
	  auto x = __bscf(mask);
	  num_active[x]++;
	  push(lanes, x, *todo);
	}

	++todo;
      }

      uint32_t ids[8];

      float dists[8];
      simd::store(length, dists);
	
      // TODO: collect stats on lane utilization to see if efficiently sorting
      // for smaller 'n' makes sense

      auto n=0;
      for (auto i=0; i<8; ++i) {
	auto num = num_active[i];
	if (num > 0) {
	  auto d = dists[i];
	  ids[n] = i;
	  for(auto j=n; j>0; --j) {
	    if (d < dists[ids[j]]) {
	      auto& a = ids[j], &b = ids[j-1];
	      a = a ^ b;
	      b = a ^ b;
	      a = a ^ b;
	    }
	  }
	  ++n;
	}
      }

      for (auto i=0; i<n; ++i) {
	push(tasks, top, node, ids[i], num_active[ids[i]]);
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
	  bvh->triangles[index].intersect2(stream, begin, num);
	  prims += accel::mbvh_t::width;
	  ++index;

	  if (prims < cur.prims) {
	    std::cout << "HANDLE THIS!" << std::endl;
	  } 
	} while(unlikely(prims < cur.prims));

	begin+=accel::mbvh_t::width;
      }
    }
  }
}

stream_mbvh_kernel_t::stream_mbvh_kernel_t(const accel::mbvh_t* bvh)
  : bvh(bvh)
{}

void stream_mbvh_kernel_t::trace(pipeline_state_t<>* state, active_t<>& active) const {
  intersect(state, active, bvh);
}
