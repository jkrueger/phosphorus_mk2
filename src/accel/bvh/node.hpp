#pragma once

#include <ImathBox.h>
#include <ImathVec.h>

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

    node_t() {
      for (int i=0; i<N*3; ++i) {
	bounds[i]       = std::numeric_limits<float>::max();
	bounds[i+(N*3)] = std::numeric_limits<float>::lowest();
      }

      memset(offset, 0, N*sizeof(uint32_t));
      memset(num, 0, N*sizeof(uint8_t));
      memset(flags, 0, N*sizeof(uint32_t));
    }

    inline void set_offset(uint32_t i, uint32_t offset) {
    }

    inline void set_bounds(uint32_t i, Imath::Box3f& bounds) {
    }

    inline void set_leaf(uint32_t i, uint32_t index, uint32_t count) {
    }
  };
}
