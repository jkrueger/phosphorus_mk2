#include "mesh.hpp"
#include "triangle.hpp"

#include <vector>

struct mesh_t::details_t {
  std::vector<Imath::V3f> vertices;
  std::vector<Imath::V3f> normals;
  std::vector<Imath::V2f> uvs;
  std::vector<uint32_t>   faces;
  std::vector<face_set_t> sets;
};

struct builder_impl_t : public mesh_t::builder_t {
  mesh_t* mesh;
  
  builder_impl_t(mesh_t* mesh)
    : mesh(mesh)
  {}

  ~builder_impl_t() {
    mesh->vertices = mesh->details->vertices.data();
    mesh->normals  = mesh->details->normals.data();
    mesh->uvs      = mesh->details->uvs.data();
    mesh->faces    = mesh->details->faces.data();
    mesh->sets     = mesh->details->sets.data();
  }

  void add_vertex(const Imath::V3f& v) {
    mesh->details->vertices.push_back(v);
  }

  void add_normal(const Imath::V3f& n) {
    mesh->details->normals.push_back(n);
  }

  void add_uv(const Imath::V2f& uv) {
    mesh->details->uvs.push_back(uv);
  }

  void add_face(uint32_t a, uint32_t b, uint32_t c) {
    mesh->details->faces.push_back(a);
    mesh->details->faces.push_back(b);
    mesh->details->faces.push_back(c);
  }

  void add_face_set(const std::vector<uint32_t>& faces) {
    mesh->details->sets.push_back({faces});
  }

  void set_normals_per_vertex() {
    mesh->flags |= mesh_t::NormalsPerVertex;
  }
  
  void set_uvs_per_vertex() {
    mesh->flags |= mesh_t::UvPerVertex;
  }
    
  void set_normals_per_vertex_per_face() {
    mesh->flags &= ~mesh_t::NormalsPerVertex;
  }

  void set_uvs_per_vertex_per_face() {
    mesh->flags &= ~mesh_t::UvPerVertex;
  }
};

mesh_t::mesh_t()
  : details(new details_t())
  , flags(UvPerVertex | NormalsPerVertex)
{}

mesh_t::~mesh_t() {
  delete details;
}

mesh_t::builder_t* mesh_t::builder() {
  return new builder_impl_t(this);
}

void mesh_t::preprocess() const {
  // get emitting light face sets, based on materials
}

void mesh_t::triangles(std::vector<triangle_t>& triangles) const {
  for (auto i=0; i<details->sets.size(); ++i) {
    for (auto j=0; j<details->sets[i].faces.size(); j++) {
      triangles.emplace_back(this, i, j);
    }
  }
}

void mesh_t::shading_parameters(pipeline_state_t<>* state, uint32_t i) const {
  const auto& face = state->shading.face[i];

  const auto u = state->shading.u[i];
  const auto v = state->shading.v[i];

  const auto w = 1 - u - v;
  const auto a = faces[face];
  const auto b = faces[face+1];
  const auto c = faces[face+2];

  auto na = a, nb = b, nc = c;

  if (!has_per_vertex_normals()) {
    na = face;
    nb = face+1;
    nc = face+2;
  }

  const auto& n0 = normals[na];
  const auto& n1 = normals[nb];
  const auto& n2 = normals[nc];

  const auto n = (w*n0+u*n1+v*n2).normalize();

  state->shading.n.x[i] = n.x;
  state->shading.n.y[i] = n.y;
  state->shading.n.z[i] = n.z;

  if (details->uvs.size() == 0) {
    state->shading.u[i] = 0;
    state->shading.v[i] = 0;
  }
  else {
    auto uva = a, uvb = b, uvc = c;

    if (!has_per_vertex_uvs()) {
      uva = face;
      uvb = face+1;
      uvc = face+2;
    }

    const auto uv0 = uvs[uva];
    const auto uv1 = uvs[uvb];
    const auto uv2 = uvs[uvc];

    const auto uv = w*uv0+u*uv1+v*uv2;

    state->shading.s[i] = uv.x;
    state->shading.t[i] = uv.y;
  }
}

triangle_t::triangle_t(const mesh_t* m, uint32_t set, uint32_t face)
  : mesh(m), set(set), face(face)
{}

uint32_t triangle_t::meshid() const {
  return mesh->id;
}

Imath::Box3f triangle_t::bounds() const {
  Imath::Box3f bounds;

  bounds.extendBy(a());
  bounds.extendBy(b());
  bounds.extendBy(c());
  
  return bounds;
}

const Imath::V3f& triangle_t::a() const {
  return mesh->details->vertices[mesh->faces[face]];
}

const Imath::V3f& triangle_t::b() const {
  return mesh->details->vertices[mesh->faces[face+1]];
}

const Imath::V3f& triangle_t::c() const {
  return mesh->details->vertices[mesh->faces[face+2]];
}
