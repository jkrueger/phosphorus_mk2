#include "scene.hpp"
#include "light.hpp"
#include "material.hpp"
#include "mesh.hpp"

#include <unordered_map>

struct scene_t::details_t {
  std::vector<mesh_t*>     meshes;
  std::vector<material_t*> materials;
  std::vector<light_t*>    lights;

  std::unordered_map<std::string, material_t*> materials_by_name;
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
  for (auto& light: details->lights) {
    delete light;
  }
  delete details;
}

void scene_t::preprocess() {
  for (auto& mesh: details->meshes) {
    mesh->preprocess(this);
  }

  for (auto& light: details->lights) {
    light->preprocess(this);
  }
}

void scene_t::triangles(std::vector<triangle_t>& out) const {
  for (auto& mesh: details->meshes) {
    mesh->triangles(out);
  }
}

void scene_t::add(light_t* light) {
  light->id = details->lights.size();
  details->lights.push_back(light);
}

void scene_t::add(mesh_t* mesh) {
  mesh->id = details->meshes.size();
  details->meshes.push_back(mesh);
}

void scene_t::add(const std::string& name, material_t* material) {
  material->id = details->materials.size();
  details->materials.push_back(material);
  details->materials_by_name[name] = material;
}

uint32_t scene_t::num_lights() const {
  return details->lights.size();
}

uint32_t scene_t::num_materials() const {
  return details->materials.size();
}

light_t* scene_t::light(uint32_t index) const {
  return details->lights[index];
}

mesh_t* scene_t::mesh(uint32_t index) const {
  return details->meshes[index];
}

material_t* scene_t::material(uint32_t index) const {
  return details->materials[index];
}

material_t* scene_t::material(const std::string& name) const {
  const auto guard = details->materials_by_name.find(name);
  if (guard != details->materials_by_name.end()) {
    return guard->second;
  }
  return nullptr;
}
