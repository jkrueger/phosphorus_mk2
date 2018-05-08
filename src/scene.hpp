#pragma once

#include "triangle.hpp"
#include "entities/camera.hpp"

#include <vector>

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
};
