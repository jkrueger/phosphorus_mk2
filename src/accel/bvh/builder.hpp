#pragma once

#include <ImathBox.h>
#include <ImathVec.h>

#include <memory>

namespace bvh {

  struct primitive_t {
    uint32_t     index;    // an index into the real scene geometry
    Imath::Box3f bounds;   // the bounding box of the primitive
    Imath::V3f   centroid; // the center of the bounding box

    inline primitive_t()
    {}

    inline primitive_t(const primitive_t& cpy)
      : index(cpy.index), bounds(cpy.bounds), centroid(cpy.centroid)
    {}

    inline primitive_t(uint32_t index, const Imath::Box3f& bounds)
      : index(index), bounds(bounds), centroid(bounds.center())
    {}
  };

  template<typename Node, typename Primitive>
  struct builder_t {
    typedef std::unique_ptr<builder_t> scoped_t;

    virtual ~builder_t() {}
    
    virtual uint32_t make_node() = 0;

    virtual Node& resolve(uint32_t n) const = 0;

    virtual uint32_t add(
      uint32_t begin
    , uint32_t end
    , const std::vector<primitive_t>& primitives
    , const std::vector<Primitive>& tiangles) = 0;
  };
}
