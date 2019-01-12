#pragma once

#include "../triangle.hpp"
#include "bvh/node.hpp"
#include "bvh/builder.hpp"
#include "math/simd.hpp"

namespace accel {

  namespace triangle {
    template<int N> struct moeller_trumbore_t;
  }

  /* this models a regular bounding volume hierarchy, but with n-wide
   * nodes, which allows to do multiple bounding volume intersections
   * at the same time */
  struct mbvh_t {
    static const uint32_t width = SIMD_WIDTH;

    typedef triangle::moeller_trumbore_t<width> triangle_t;
    typedef bvh::builder_t<mbvh::node_t<width>, ::triangle_t> builder_t;

    struct details_t;

    details_t* details;

    // pointer to the beginning of the bvh data structure. this is
    // ultimately a flat array of nodes, which represents a linearized
    // tree of nodes
    mbvh::node_t<width>* root;
    // the number of nodes in the tree
    uint32_t num_nodes;
    // the optimized triangle data structures in the tree. nodes point into this
    // array, if they are leaf nodes
    triangle_t* triangles;
    // number of triangles in the tree
    uint32_t num_triangles;

    mbvh_t();
    ~mbvh_t();

    /** clear all data from the bvh */
    void reset();

    builder_t* builder();

    // the bounds of the mesh data in this accelerator
    Imath::Box3f bounds() const;
  };
}
