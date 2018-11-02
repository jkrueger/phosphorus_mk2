#pragma once

#include <ImathBox.h>
#include <ImathVec.h>

struct mesh_t;

/* identifies a tiangle in the mesh. mostly for use in acceleration
 * data structure construction */
struct triangle_t {
  const mesh_t*  mesh;
  const uint32_t set;
  const uint32_t face;

  triangle_t(const mesh_t* m, uint32_t set, uint32_t face);

  uint32_t meshid() const;
  
  float area() const;

  Imath::Box3f bounds() const;

  Imath::V2f sample(const Imath::V2f& uv) const;

  const Imath::V3f& a() const;
  const Imath::V3f& b() const;
  const Imath::V3f& c() const;

  Imath::V3f barycentric_to_point(const Imath::V2f& uv) const;
};
