#include "light.hpp"
#include "material.hpp"
#include "mesh.hpp"
#include "scene.hpp"
#include "math/sampling.hpp"

#include <algorithm>
#include <random> 

struct area_light_t : public light_t::details_t {
  mesh_t* mesh;
  uint32_t set;
  std::vector<triangle_t> triangles;
  float area;
  float* cdf;

  area_light_t(mesh_t* mesh, uint32_t set)
    : mesh(mesh)
    , set(set)
    , details_t(mesh->material(set))
    , area(0.0f)
  {
    mesh->triangles(set, triangles);
  }

  ~area_light_t() {
    delete cdf;
  }

  void preprocess(const scene_t* scene) {
    // compute cdf based on the area of each triangle of
    // the face set that defines the surface of this light source
    cdf = new float[triangles.size()];

    const auto num = triangles.size();

    for (auto i=0; i<num; ++i) {
      area += triangles[i].area();
      cdf[i] = area;
    }

    for (auto i=0; i<num; ++i) {
      cdf[i] /= area;
    }

    std::cout << "AREA: " << area << std::endl;
  }

  void sample(const Imath::V2f& uv, sampler_t::light_sample_t& out) const {
    const auto num = triangles.size();

    // auto i = 0;
    // while (i < (num-1) && uv.x > cdf[i]) {
    //   ++i;
    // }

    const auto i = std::min((size_t)std::floor(uv.x * num), num - 1);

    const auto one_minus_epsilon =
      1.0f-std::numeric_limits<float>::epsilon();
    const auto remapped =
      std::min(uv.x * num - i, one_minus_epsilon);

    const auto& triangle    = triangles[i];
    const auto  barycentric = triangle_t::sample({remapped, uv.y});

    out.p    = triangle.barycentric_to_point(barycentric);
    out.uv   = barycentric;
    out.pdf  = 1.0f / (num * triangle.area());
    out.mesh = triangle.meshid() | (triangle.matid() << 16);
    out.face = triangle.face;
  }

  void sample(
    const soa::vector2_t<sampler_t::light_samples_t::step>& uv
  , sampler_t::light_samples_t& out) const
  {
  //   const auto one = simd::floatv_t(1.0f);
  //   const auto num = simd::floatv_t(triangles.size());

  //   auto x = simd::floatv_t(uv.x);
  //   auto y = simd::floatv_t(uv.y);

  //   const auto indices = simd::min(simd::floor(x * num), num - one);

  //   const auto e = std::numeric_limits<float>::epsilon();
  //   const auto one_minus_epsilon = simd::floatv_t(1.0f-e);

  //   x = simd::min(x * num, one_minus_epsilon);

  //   const auto barycentrics = triangle_t::sample(x, y);

  //   out.p = mesh->barycentrics_to_point(set, indices, barycentrics);
  //   out.u = barycentrics.x;
  //   out.v = barycentrics.y;
  //   out.pdf = simd::floatv_t(1.0f / area);
  //   // out.mesh = simd::int32v_t(mesh->id | (matid << 16));
  //   // out.face = mesh->face_ids(set, indices);
  }  
};

struct infinite_light_t : public light_t::details_t {
  infinite_light_t(uint32_t matid)
    : details_t(matid)
  {}

  void sample(const Imath::V2f& uv, sampler_t::light_sample_t& out) const  {
    Imath::V3f sampled;
    sample::hemisphere::uniform(uv, sampled, out.pdf);

    out.p = sampled * 1000.0f;
    out.mesh = matid << 16;
  }
};

light_t::light_t(type_t type, details_t* details)
  : type(type), details(details)
{}

light_t::~light_t() {
  delete details;
}

void light_t::preprocess(const scene_t* scene) {
  switch(type) {
  case AREA:
    static_cast<area_light_t*>(details)->preprocess(scene);
    break;
  default:
    // nothing to do for other light sources
    break;
  }
}

void light_t::sample(
  const Imath::V2f& uv
, sampler_t::light_sample_t& out) const
{
  switch(type) {
  case POINT:
    break;
  case AREA:
    static_cast<const area_light_t*>(details)->sample(uv, out);
    break;
  case INFINITE:
    static_cast<const infinite_light_t*>(details)->sample(uv, out);
    break;
  }
}

void light_t::sample(
  const soa::vector2_t<sampler_t::light_samples_t::step>& uv
, sampler_t::light_samples_t& out) const
{
  switch(type) {
  case POINT:
    break;
  case AREA:
    static_cast<const area_light_t*>(details)->sample(uv, out);
    break;
  case INFINITE:
    // static_cast<const infinite_light_t*>(details)->sample(uv, out);
    printf("SIMD sampling not implemented on infinite lights");
    break;
  }
}

light_t* light_t::make_area(mesh_t* mesh, uint32_t set) {
  auto details = new area_light_t(mesh, set);
  return new light_t(AREA, details);
}

light_t* light_t::make_infinite(const material_t* material) {
  auto details = new infinite_light_t(material->id);
  return new light_t(INFINITE, details);
}
