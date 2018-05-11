#include "bvh.hpp"
#include "bvh/builder.hpp"
#include "bvh/node.hpp"
#include "triangle.hpp"

#include <vector>

namespace accel {
  struct mbvh_t::details_t {
    typedef mbvh::node_t<mbvh_t::width> node_t;
    typedef triangle::moeller_trumbore_t<mbvh_t::width> triangle_t;

    std::vector<node_t> nodes;
    std::vector<triangle_t> triangles;
  };

  struct builder_t :
    public bvh::builder_t<
      mbvh_t::details_t::node_t
    , triangle_t
    >
  {
    mbvh_t* bvh;

    std::vector<mbvh_t::details_t::node_t>& nodes;
    std::vector<mbvh_t::details_t::triangle_t>& triangles;

    builder_t(mbvh_t* bvh)
      : bvh(bvh)
      , nodes(bvh->details->nodes)
      , triangles(bvh->details->triangles) {
    }

    virtual ~builder_t() {
      bvh->root      = nodes.data();
      bvh->triangles = triangles.data();
    }

    uint32_t make_node() {
      nodes.emplace_back(/* constructor args would go here */);
      return nodes.size() - 1;
    }
 
    mbvh_t::details_t::node_t& resolve(uint32_t n) const {
      return nodes[n];
    }

    uint32_t add(
      uint32_t begin
    , uint32_t end
    , const std::vector<bvh::primitive_t>& primitives
    , const std::vector<triangle_t>& things)
    {
      uint32_t off = triangles.size();
      uint32_t num = end - begin;

      for (auto i=begin; i<end; i+=mbvh_t::width) {
	triangles.emplace_back(things.data() + i, num);
      }

      return off;
    }
  };

  mbvh_t::mbvh_t()
    : details(new details_t()) {
  }

  mbvh_t::~mbvh_t() {
    delete details;
  }

  mbvh_t::builder_t* mbvh_t::builder() {
    return new accel::builder_t(this);
  }
}
