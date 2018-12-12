#include "mesh.hpp"
#include "light.hpp"
#include "material.hpp"
#include "scene.hpp"
#include "triangle.hpp"

#include <cmath>
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

  void add_face_set(const material_t* material, const std::vector<uint32_t>& faces) {
    mesh->details->sets.push_back({material->id, faces});
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

void mesh_t::preprocess(scene_t* scene) {
  // get emitting light face sets, based on materials
  for (auto i=0; i<details->sets.size(); ++i) {
    const auto material = scene->material(details->sets[i].material);
    if (material->is_emitter()) {
      scene->add(light_t::make(this, i));
    }
  }
}

void mesh_t::triangles(std::vector<triangle_t>& out) const {
  for (auto i=0; i<details->sets.size(); ++i) {
    triangles(i, out);
  }
}

void mesh_t::triangles(uint32_t set, std::vector<triangle_t>& out) const {
  for (auto j=0; j<details->sets[set].faces.size(); j++) {
    out.emplace_back(this, set, details->sets[set].faces[j]*3);
  }
}

void mesh_t::shading_parameters(
  const ray_t<>* rays
, interaction_t<>* hits
, uint32_t i) const
{
  Imath::V3f n;
  Imath::V2f st;

  shading_parameters(rays, n, st, i);

  hits->n.from(i, n);
  hits->s[i] = st.x;
  hits->t[i] = st.y;
}

void mesh_t::shading_parameters(
  const ray_t<>* rays     
, Imath::V3f& n
, Imath::V2f& st
, uint32_t i) const
{
  const auto& face = rays->face[i];

  const auto u = rays->u[i];
  const auto v = rays->v[i];

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

  n = (w*n0+u*n1+v*n2).normalize();

  if (details->uvs.size() == 0) {
    st = Imath::V2f(0);
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

    st = w*uv0+u*uv1+v*uv2;
  }
}

triangle_t::triangle_t(const mesh_t* m, uint32_t set, uint32_t face)
  : mesh(m)
  , set(set)
  , face(face)
{}

uint32_t triangle_t::meshid() const {
  return mesh->id;
}

uint32_t triangle_t::matid() const {
  return mesh->sets[set].material;
}

Imath::Box3f triangle_t::bounds() const {
  Imath::Box3f bounds;

  bounds.extendBy(a());
  bounds.extendBy(b());
  bounds.extendBy(c());
  
  return bounds;
}

float triangle_t::area() const {
  const auto ab = b() - a();
  const auto ac = c() - a();

  const auto z = ab.cross(ac);

  return 0.5f * z.length();
}

const Imath::V3f& triangle_t::a() const {
  return mesh->vertices[mesh->faces[face]];
}

const Imath::V3f& triangle_t::b() const {
  return mesh->vertices[mesh->faces[face+1]];
}

const Imath::V3f& triangle_t::c() const {
  return mesh->vertices[mesh->faces[face+2]];
}

Imath::V3f triangle_t::barycentric_to_point(const Imath::V2f& uv) const {
  return uv.x * a() + uv.y * b() + (1-uv.x-uv.y) * c();
}

Imath::V2f triangle_t::sample(const Imath::V2f& uv) const {
  const auto x = std::sqrt(uv.x);
  const auto u = 1 - x;
  const auto v = uv.y * x;

  return {u, v};
}
