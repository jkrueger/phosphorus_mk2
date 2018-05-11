#pragma once

#include "builder.hpp"
#include "node.hpp"
#include "math/aabb.hpp"
#include "math/simd.hpp"

#include <ImathBox.h>
#include <ImathVec.h>

#include <algorithm>
#include <iostream>
#include <tuple>
#include <vector>

namespace bvh {
  static const uint8_t MAX_PRIMS_IN_NODE = SIMD_WIDTH;
  static const uint8_t NUM_SPLIT_BINS    = 12;

  /**
   * Build information about a subset of the primitives in the scene
   */
  struct geometry_t {
    // information relevant for the build, about the primtivies in the scene
    std::vector<primitive_t>& primitives;
    // indices into the primitives vector
    uint32_t start, end;
    // the bounding volume for this subset of the primitives in the scene
    Imath::Box3f bounds;
    Imath::Box3f centroid_bounds;

    inline geometry_t(const geometry_t& parent)
      : primitives(parent.primitives)
      , start(0)
      , end(0)
    {}

    inline geometry_t(
        std::vector<primitive_t>& primitives
      , uint32_t start
      , uint32_t end)
      : primitives(primitives), start(start), end(end)
    {
      for (auto i=start; i<end; ++i) {
	bounds.extendBy(primitives[i].bounds);
	centroid_bounds.extendBy(primitives[i].centroid);
      }
    }

    inline geometry_t& operator=(const geometry_t& g) {
      primitives      = g.primitives;
      start           = g.start;
      end             = g.end;
      bounds          = g.bounds;
      centroid_bounds = g.centroid_bounds;
      return *this;
    }

    inline uint32_t count() const {
      return end - start;
    }

    inline const primitive_t& primitive(uint32_t i) const {
      return primitives[start+i];
    }

    template<typename F>
    inline void partition(const F& f, geometry_t& l, geometry_t& r) {
      auto p   = std::partition(&primitives[start], &primitives[end-1]+1, f);
      auto mid = (uint32_t) (p - &primitives[0]);

      l = {primitives, start, mid};
      r = {primitives, mid, end};
    }
  };

  struct split_t {
    uint32_t axis;
    uint32_t bin;
    float    cost;

    inline split_t(uint32_t axis, uint32_t bin, float cost)
      : axis(axis), bin(bin), cost(cost)
    {}
  };

  struct bin_t {
    uint32_t     count;
    Imath::Box3f bounds;

    inline bin_t()
      : count(0)
    {}

    inline void add(const primitive_t& p) {
      bounds.extendBy(p.bounds);
      count++;
    }
  };

  template<int N>
  struct bins_t {
    bin_t bins[N];

    inline const bin_t& operator[](uint32_t i) const {
      return bins[i];
    }

    inline void add(const Imath::Box3f& bounds, const primitive_t& p, uint8_t axis) {
      bins[find(bounds, p, axis)].add(p);
    }

    static inline Imath::V3f offset(const Imath::Box3f& l, const Imath::V3f& r) {
      auto o = r - l.min;
      if (l.max.x > l.min.x) { o.x /= (l.max.x - l.min.x); } 
      if (l.max.y > l.min.y) { o.y /= (l.max.y - l.min.y); } 
      if (l.max.z > l.min.z) { o.z /= (l.max.z - l.min.z); }
      return o;
    }

    static inline uint32_t find(const Imath::Box3f& bounds, const primitive_t& p, uint8_t axis) {
      auto off = offset(bounds, p.centroid)[axis];
      return std::min((int) (N * off), N-1);
    }
  };

  struct node_t {
    geometry_t primitives; // the  primitives in this node
  };

  inline bool too_large(const geometry_t& g) {
    return g.count() > MAX_PRIMS_IN_NODE;
  }

  inline bool too_small_to_split(const geometry_t& g) {
    return g.count() < MAX_PRIMS_IN_NODE;
  }

  inline float leaf_cost(const split_t& s, const geometry_t& g) {
    return g.count();
  }

  split_t find(const geometry_t& geometry) {
    auto best_axis = 0;
    auto best_bin  = 0;
    auto best_cost = std::numeric_limits<float_t>::max();

    for (auto axis=0; axis<3; ++axis) {
      bins_t<NUM_SPLIT_BINS> bins;

      if (aabb::is_empty_on(geometry.centroid_bounds, axis)) {
	continue;
      }

      for (auto i=0; i<geometry.count(); ++i) {
	bins.add(geometry.centroid_bounds, geometry.primitive(i), axis);
      }

      auto split_cost = std::numeric_limits<float_t>::max();
      auto split_bin  = 0;

      for (auto i=0; i<NUM_SPLIT_BINS-1; ++i) {
	Imath::Box3f a, b;
	auto left = 0; auto right = 0;
	for (auto j=0; j<=i; ++j) {
	  a.extendBy(bins[j].bounds);
	  left += bins[j].count;
	}

	for (auto j=i+1; j<NUM_SPLIT_BINS; ++j) {
	  b.extendBy(bins[j].bounds);
	  right += bins[j].count;
	}

	auto cost = (left * aabb::area(a) + right * aabb::area(b)) / aabb::area(geometry.bounds);
	if (cost < split_cost) {
	  split_cost = cost;
	  split_bin = i;
	}
      }

      if (split_cost < best_cost) {
	best_axis = axis;
	best_cost = split_cost;
	best_bin  = split_bin;
      }
    }
    return split_t(best_axis, best_bin, best_cost);
  }

  void split(const split_t& split, geometry_t& parent, geometry_t& l, geometry_t& r) {
    parent.partition([&](const primitive_t& p) {
	auto bin = bins_t<NUM_SPLIT_BINS>::find(parent.centroid_bounds, p, split.axis);
	return bin <= split.bin; 
      }, l, r);
  }

  int32_t largest_node(const geometry_t* node, uint32_t n) {
    int32_t out = -1;
    float   a   = std::numeric_limits<float>::max(); 
    for (auto i=0; i<n; ++i) {
      if (node[i].count() < MAX_PRIMS_IN_NODE) {
	continue;
      }

      auto node_area = aabb::area(node[i].bounds);
      if (node_area < a) {
	out = i;
	a   = node_area;
      }
    }
    return out;
  }

  template<typename Things, typename Builder>
  uint32_t from(geometry_t& geometry, const Things& things, Builder& bvh) {
    auto n = geometry.count();
    auto s = find(geometry);

    if (too_small_to_split(geometry) || leaf_cost(s, geometry) <= 1.0f + s.cost) {
      return 0;
    }

    auto num_children = 2;
    geometry_t children[8] = { [0 ... 7] = { geometry } };

    split(s, geometry, children[0], children[1]);

    while (num_children < 8) {
      auto split_child = largest_node(children, num_children);
      if (split_child == -1) {
	break;
      }
      auto s = find(children[split_child]);
      // check sha heuristic?
      geometry_t tmp(geometry);
      split(s, children[split_child], tmp, children[num_children]);
      children[split_child] = tmp;

      ++num_children;
    }

    // make a new node in the BVH
    auto node_index = bvh->make_node();

    int32_t child_indices[num_children];
    for (int i=0; i<num_children; ++i) {
      child_indices[i] = from(children[i], things, bvh);
    }

    auto& node = bvh->resolve(node_index);
    for (int i=0; i<num_children; ++i) {
      node.set_bounds(i, children[i].bounds);

      if (child_indices[i]) {
	node.set_offset(i, child_indices[i]);
      }
      else {
	auto index =
	  bvh->add(
	    children[i].start, children[i].end,
	    geometry.primitives,
	    things);
	node.set_leaf(i, index, children[i].count());
      }
    }

    return node_index;
  }

  template<typename Things, typename Builder>
  void from(Builder& bvh, const Things& things) {
    std::vector<primitive_t> primitives(things.size());
    for (uint32_t i=0; i<things.size(); ++i) {
      primitives[i] = { i, things[i].bounds() };
    }

    geometry_t geometry(primitives, 0, primitives.size());
    from(geometry, things, bvh);
  }
}
