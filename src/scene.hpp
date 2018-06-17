#pragma once

#include "triangle.hpp"
#include "entities/camera.hpp"

#include <vector>

struct material_t;
struct mesh_t;
struct triangle_t;

struct scene_t {
  struct details_t;

  details_t* details;
  
  camera_t camera;

  scene_t();
  ~scene_t();

  void preprocess();

  void triangles(std::vector<triangle_t>& v) const;

  void add(mesh_t* mesh);

  void add(material_t* mesh);
  
  uint32_t num_materials() const;

  mesh_t* mesh(uint32_t index) const;

  material_t* material(uint32_t index) const;
};
