#pragma once

#include "triangle.hpp"
#include "entities/camera.hpp"

#include <string>
#include <vector>

struct light_t;
struct material_t;
struct mesh_t;
struct triangle_t;

struct scene_t {
  struct details_t;

  details_t* details;
  
  camera_t camera;

  scene_t();
  ~scene_t();

  void reset();

  void preprocess();

  void triangles(std::vector<triangle_t>& v) const;

  void add(light_t* light);

  void add(mesh_t* mesh);

  void add(const std::string& name, material_t* mesh);

  uint32_t num_lights() const;

  uint32_t num_meshes() const;
  
  uint32_t num_materials() const;

  light_t* light(uint32_t index) const;

  light_t* environment() const;

  mesh_t* mesh(uint32_t index) const;

  material_t* material(uint32_t index) const;
  material_t* material(const std::string& name) const;
};
