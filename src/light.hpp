#pragma once

#include "sampling.hpp"

struct mesh_t;
struct scene_t;

struct light_t {
  struct details_t;
  details_t* details;

  uint32_t id;

  light_t(details_t* details);
  ~light_t();

  /* precompute data to effectively sample the light source */
  void preprocess(scene_t* scene);

  /* sample a point on the light source */
  void sample(const Imath::V2f& uv, sampler_t::light_sample_t& out) const;

  /* constructs a light source from a face set of a mesh */
  static light_t* make(mesh_t* m, uint32_t set);
};
