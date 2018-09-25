#pragma once

struct mesh_t;
struct scene_t;

struct light_t {
  struct details_t* details;

  light_t(details_t* details);

  void preprocess(scene_t* scene);

  /* constructs a light source from a face set of a mesh */
  static light_t* make(mesh_t* m, uint32_t set);
};
