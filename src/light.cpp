#include "light.hpp"
#include "material.hpp"
#include "mesh.hpp"
#include "scene.hpp"

#include <random> 

struct light_t::details_t {
  std::vector<triangle_t> triangles;
  float area;
  float* cdf;

  details_t()
    : area(0.0f)
  {}

  ~details_t() {
    delete cdf;
  }
};

light_t::light_t(details_t* details)
  : details(details)
{}

light_t::~light_t() {
  delete details;
}

void light_t::preprocess(scene_t* scene) {
  // compute cdf based on the area of each triangle of
  // the face set that defines the surface of this light source
  details->cdf = new float[details->triangles.size()];

  const auto num = details->triangles.size(); 
  
  for (auto i=0; i<num; ++i) {
    details->area += details->triangles[i].area();
    details->cdf[i] = details->area;
  }

  for (auto i=0; i<num; ++i) {
    details->cdf[i] /= details->area;
  }
}

void light_t::sample(const Imath::V2f& uv, sampler_t::light_sample_t& out) const {
  const auto num = details->triangles.size();

  auto i = 0;
  while (i < (num-1) && uv.x < details->cdf[i]) {
    ++i;
  }

  const auto one_minus_epsilon =
    1.0f-std::numeric_limits<float>::epsilon();
  const auto remapped =
    std::min(uv.x * num - i, one_minus_epsilon);

  const auto& triangle    = details->triangles[i];
  const auto  barycentric = triangle.sample({remapped, uv.y});

  out.p    = triangle.barycentric_to_point(barycentric);
  out.uv   = barycentric;
  out.pdf  = 1.0f / details->area;
  out.mesh = triangle.meshid() | (triangle.matid() >> 16);
  out.face = triangle.face;
}

light_t* light_t::make(mesh_t* mesh, uint32_t set) {
  details_t* details = new details_t();
  mesh->triangles(set, details->triangles);

  return new light_t(details);
}
