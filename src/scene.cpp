#include "scene.hpp"
#include "mesh.hpp"

struct scene_t::details_t {
  std::vector<mesh_t*> meshes;
  // lights
  // materials
};

scene_t::scene_t()
  : details(new details_t()) {
}

scene_t::~scene_t() {
  printf("DTORX: %d\n", details->meshes.size());
  for (auto& mesh : details->meshes) {
    delete mesh;
  }
  delete details;
}

void scene_t::preprocess() {
  for (auto& mesh: details->meshes) {
    mesh->preprocess(/*, lights */);
  }
}

void scene_t::triangles(std::vector<triangle_t>& out) const {
  printf("TRIS: %p, %d\n", this, details->meshes.size());
  for (auto& mesh: details->meshes) {
    mesh->triangles(out);
  }
}

void scene_t::add(mesh_t* mesh) {
  printf("ADD: %p, %d\n", this, details->meshes.size());
  mesh->id = details->meshes.size();
  details->meshes.push_back(mesh);
}
