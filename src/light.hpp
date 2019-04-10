#pragma once

#include "sampling.hpp"

struct material_t;
struct mesh_t;
struct scene_t;

struct light_t {
  enum type_t {
    POINT,
    AREA,
    INFINITE
  } type;

  struct details_t {
    uint32_t matid;

    details_t(uint32_t matid)
      : matid(matid)
    {}
  } *details;

  uint32_t id;

  light_t(type_t type, details_t* details);
  ~light_t();

  /* precompute data to effectively sample the light source */
  void preprocess(const scene_t* scene);

  /* sample a point on the light source */
  void sample(
    const Imath::V2f& uv
  , sampler_t::light_sample_t& out) const;

  /* sample n points on a light, where n is the simd width of 
   * the cpu */
  void sample(
    const soa::vector2_t<SIMD_WIDTH>& uv
  , sampler_t::light_samples_t<SIMD_WIDTH>& out) const;

  inline bool is_area() const {
    return type == AREA;
  }
  
  inline bool is_infinite() const {
    return type == INFINITE;
  }
  
  /* get the material for this light source */
  inline uint32_t matid() const {
    return details->matid;
  }

  /* constructs an area light source from a face set of a mesh */
  static light_t* make_area(mesh_t* m, uint32_t set);

  /* constructs an infinite light from a material */
  static light_t* make_infinite(const material_t* material);
};
