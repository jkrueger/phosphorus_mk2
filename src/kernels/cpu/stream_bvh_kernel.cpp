#include "stream_bvh_kernel.hpp"
#include "detail/stream.hpp"
#include "accel/bvh.hpp"
#include "accel/triangle.hpp"
#include "math/simd/aabb.hpp"
#include "utils/compiler.hpp"

template<typename Stream>
void intersect(Stream& stream, const active_t<>& active, const accel::mbvh_t* bvh) {
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

      uint32_t indices[] = {
	0, 8, 16, 24, 32, 40
      };

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

	const auto idx0 = *todo ^ ~(accel::mbvh_t::width-1);
	const auto idx1 = *todo ^ (accel::mbvh_t::width-1);

	const auto& ray = stream.rays[idx0];

	auto hits =
	  simd::intersect<accel::mbvh_t::width>(
	    bounds
	  , simd::vector3_t(ray.p.x[idx1], ray.p.y[idx1], ray.p.z[idx1])
	  , simd::vector3_t(ray.wi.x[idx1], ray.wi.y[idx1], ray.wi.z[idx1]).rcp()
	  , simd::load(ray.d[idx1])
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
      auto todo = pop(lanes, cur.lane, cur.num_rays);
      auto end  = todo + cur.num_rays;
      do {
	const auto index   = cur.offset;
	const auto offsets = simd::load(todo);
	do {
	  const auto mask =
	    bvh->triangles[index].intersect<SIMD_WIDTH>(/* ray packet */);
	  while(mask != 0) {
	    const auto x    = *todo + __bscf(mask);
	    const auto idx0 = x & ~(accel::mbvh_t::width-1);
	    const auto idx1 = x & (accel::mbvh_t::width-1);

	    stream.shading[idx0].mesh[idx1] = bvh->triangles[index].meshid[x];
	    stream.shading[idx0].set[idx1]  = bvh->triangles[index].setid[x];
	    stream.shading[idx0].face[idx1] = bvh->triangles[index].faceid[x];
	  }
	  index += accel::mbvh_t::width;
	} while(index < cur.prims);
      } while((todo+=accel::mbvh_t::width) != end);
    }
  }
}

stream_mbvh_kernel_t::stream_mbvh_kernel_t(const accel::mbvh_t* bvh)
  : bvh(bvh)
{}

void stream_mbvh_kernel_t::trace(pipeline_state_t<>& state, active_t<>& active) const {
  intersect(state, active, bvh);
}
