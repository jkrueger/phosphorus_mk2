#pragma once

#include "state.hpp"
#include "triangle.hpp"

#include <ImathVec.h>

#include <vector>

struct material_t;
struct scene_t;

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
    uint32_t  material;
    uint32_t  num_faces;
    uint32_t* faces;

    inline face_set_t(uint32_t material, const std::vector<uint32_t>& _faces)
      : material(material)
      , num_faces(_faces.size())
    {
      posix_memalign((void**) &faces, 32, _faces.size() * sizeof(uint32_t));
      memcpy(faces, _faces.data(), _faces.size() * sizeof(uint32_t));
    }

    inline uint32_t mat() const {
      return material;
    }
  };

  /* helper stuff to construct a mesh. mostly just here to hide
   * part of the implementation from you */
  struct builder_t {
    typedef std::unique_ptr<builder_t> scoped_t;

    virtual ~builder_t()
    {}

    virtual void add_vertex(const Imath::V3f& v) = 0;
    virtual void add_normal(const Imath::V3f& n) = 0;
    virtual void add_tangent(const Imath::V3f& t) = 0;
    virtual void add_uv(const Imath::V2f& uv) = 0;
    virtual void add_face(uint32_t a, uint32_t b, uint32_t c, bool smooth = true) = 0;
    virtual void add_face_set(uint32_t id, const std::vector<uint32_t>& faces) = 0;
    virtual void add_face_set(const material_t* m, const std::vector<uint32_t>& faces) = 0;

    virtual void set_normals_per_vertex() = 0;
    virtual void set_uvs_per_vertex() = 0;

    virtual void set_normals_per_vertex_per_face() = 0;
    virtual void set_uvs_per_vertex_per_face() = 0;
  };

  Imath::V3f* vertices;
  Imath::V3f* normals;
  Imath::V3f* tangents;
  Imath::V2f* uvs;
  uint32_t*   faces;
  face_set_t* sets;

  uint32_t id;
  uint32_t flags;

  mesh_t();
  ~mesh_t();

  builder_t* builder();
  
  // builder_t* builder(
  //   uint32_t num_vertices
  // , uint32_t num_normals
  // , uint32_t num_uvs
  // , uint32_t num_faces);

  /* allocate extra space for precomputed tangents, which might
   * comes from external modelling packages */
  void allocate_tangents();

  /* preprocess the mesh based on  */
  void preprocess(scene_t* scene);

  /* get a list of descriptors of the triangles in this mesh */
  void triangles(std::vector<triangle_t>& triangle) const;

  /* get a list of triangles that share the same material */
  void triangles(uint32_t set, std::vector<triangle_t>& triangle) const;

  simd::vector3v_t barycentrics_to_point(
    uint32_t setid
  , const simd::int32v_t& indices                                            
  , const simd::vector3v_t& barycentrics) const;

  /** converts an index into a face set into actual face indices in the mesh */
  simd::int32v_t face_ids(uint32_t setid, const simd::int32v_t& indices) const;

  /* fill in the shading parameters in the state from this mesh */
  void shading_parameters(
    const ray_t<>* rays     
  , interaction_t<>* hits
  , uint32_t i) const;

  void shading_parameters(
    const ray_t<>* rays     
  , Imath::V3f& n
  , Imath::V2f& st
  , uint32_t i) const;

  inline bool has_per_vertex_normals() const {
    return (flags & NormalsPerVertex) != 0;
  }

  inline bool has_per_vertex_uvs() const {
    return (flags & UvPerVertex) != 0;
  }

  inline uint32_t material(uint32_t set) const {
    return sets[set].mat();
  }
};
