#pragma once

#include <ImathBox.h>
#include <ImathVec.h>

#include <limits>

#include <string.h>

namespace mbvh {
  template<int N>
  struct node_t {
    // bounding volume hierachies of this node
    float bounds[2*N*3];
    // offset to right child node, or offset into a
    // list of triangles if this is a leaf node
    uint32_t offset[N];
    // number of primitives stored at a node, if it is a child node
    uint8_t num[N];
    // a set of flags attached to a node
    uint32_t flags[N];
    // keep alignment
    uint8_t pad[24];

    node_t() {
      for (int i=0; i<N*3; ++i) {
	      bounds[i]       = std::numeric_limits<float>::max();
	      bounds[i+(N*3)] = std::numeric_limits<float>::lowest();
      }

      memset(offset, 0, N*sizeof(uint32_t));
      memset(num, 0, N*sizeof(uint8_t));
      memset(flags, 0, N*sizeof(uint32_t));
    }

    inline void set_offset(uint32_t i, uint32_t n) {
      offset[i] = n;
    }

    inline void set_bounds(uint32_t i, Imath::Box3f& b) {
      bounds[i     ] = b.min.x;
      bounds[i +  8] = b.min.y;
      bounds[i + 16] = b.min.z;
      bounds[i + 24] = b.max.x;
      bounds[i + 32] = b.max.y;
      bounds[i + 40] = b.max.z;
    }

    inline Imath::Box3f get_bounds() const {
      Imath::Box3f bounds;
      for (auto i=0; i<N; ++i) {
	bounds.extendBy(get_bounds(i));
      }
      return bounds;
    }

    inline Imath::Box3f get_bounds(uint32_t i) const {
      return Imath::Box3f(
        Imath::V3f(bounds[i], bounds[i+8], bounds[i+16]),
	Imath::V3f(bounds[i+24], bounds[i+32], bounds[i+40]));
    }

    inline void set_leaf(uint32_t i, uint32_t index, uint32_t count) {
      flags[i]  = 0x1;
      offset[i] = index;
      num[i]    = count;
    }
  };
}
