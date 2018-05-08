#pragma once

#include "triangle.hpp"

#include <ImathVec.h>

#include <vector>

/* a mesh models a 3d obect in the scene, with a set of materials 
 * applied to it */
struct mesh_t {
  struct details_t;

  details_t* details;

  enum flags_t {
    UvPerVertex      = 1,
    NormalsPerVertex = 2
  };

  /* a face set applies a material to a sub mesh */
  struct face_set_t {
    std::vector<uint32_t> faces;
    // TODO: handle to material for the set
  };

  /* helper stuff to construct a mesh. mostly just here to hide
   * part of the implementation from you */
  struct builder_t {
    typedef std::unique_ptr<builder_t> scoped_t;

    virtual ~builder_t()
    {}

    virtual void add_vertex(const Imath::V3f& v) = 0;
    virtual void add_normal(const Imath::V3f& n) = 0;
    virtual void add_uv(const Imath::V2f& uv) = 0;
    virtual void add_face(uint32_t a, uint32_t b, uint32_t c) = 0;
    virtual void add_face_set(const std::vector<uint32_t>& faces) = 0;

    virtual void set_normals_per_vertex() = 0;
    virtual void set_uvs_per_vertex() = 0;
    
    virtual void set_normals_per_vertex_per_face() = 0;
    virtual void set_uvs_per_vertex_per_face() = 0;
  };

  Imath::V3f* vertices;
  Imath::V3f* normals;
  Imath::V2f* uvs;
  uint32_t*   faces;

  uint32_t id;
  uint32_t flags;

  mesh_t();
  ~mesh_t();

  builder_t* builder();

  void preprocess() const;

  /* get a list of descriptors of the triangles in this mesh */
  void triangles(std::vector<triangle_t>& triangle) const;
};
