#include "scene.hpp"
#include "material.hpp"
#include "mesh.hpp"

struct scene_t::details_t {
  std::vector<mesh_t*>     meshes;
  std::vector<material_t*> materials;
  // emitters
};

scene_t::scene_t()
  : details(new details_t()) {
}

scene_t::~scene_t() {
  for (auto& mesh : details->meshes) {
    delete mesh;
  }
  for (auto& material : details->materials) {
    delete material;
  }
  delete details;
}

void scene_t::preprocess() {
  for (auto& mesh: details->meshes) {
    mesh->preprocess(/*, lights */);
  }
}

void scene_t::triangles(std::vector<triangle_t>& out) const {
  for (auto& mesh: details->meshes) {
    mesh->triangles(out);
  }
}

void scene_t::add(mesh_t* mesh) {
  mesh->id = details->meshes.size();
  details->meshes.push_back(mesh);
}

void scene_t::add(material_t* material) {
  material->id = details->materials.size();
  details->materials.push_back(material);
}

uint32_t scene_t::num_materials() const {
  return details->materials.size();
}

mesh_t* scene_t::mesh(uint32_t index) const {
  return details->meshes[index];
}

material_t* scene_t::material(uint32_t index) const {
  return details->materials[index];
}
