#include "light.hpp"
#include "material.hpp"
#include "mesh.hpp"
#include "scene.hpp"

struct light_t::details_t {
  uint32_t material;
  std::vector<triangle_t> triangles;
};

light_t::light_t(details_t* details)
  : details(details)
{}

light_t::~light() {
}

void light_t::preprocess(scene_t* scene) {
  // compute cdf based on the area of each triangle of
  // the face set that defines the surface of this light source

  float cdf[details->triangles.size()];
  float area = 0.0f;

  const auto num = details->triangles.size(); 
  
  for (auto i=0; i<num; ++i) {
    area += details->triangles[i].area();
    cdf[i] = area;
  }

  for (auto i=0; i<num; ++i) {
    cdf[i] /= area;
  }

  // generate sample points on light

  
}

light_t* light_t::make(mesh_t* mesh, uint32_t set) {
  details_t* details = new details_t(mesh->material(set));
  mesh->triangles(set, details->triangles);

  return new light_t(details);
}
