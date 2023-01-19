#pragma once

#include "sampling.hpp"

struct material_t;
struct mesh_t;
struct scene_t;

struct light_t {
  enum type_t {
    POINT, // simple point light type
    DISTANT, // distant light source (i.e. sun light)
    RECT, // simple rectangular area light 
    AREA, // mesh backed area light
    INFINITE // infinite area light (i.e. sampled environment light)
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

  /* compute a shadow ray direction for a light sample */
  Imath::V3f setup_shadow_ray(
    const Imath::V3f& p
  , const sampler_t::light_sample_t& sample) const;

  /* evaluate the emitted light at a sample */
  Imath::V3f le(const scene_t& scene, const sampler_t::light_sample_t& sample, const Imath::V3f& wi) const;

  inline bool is_point() const {
    return type == POINT;
  }

  inline bool is_distant() const {
    return type == DISTANT;
  }

  inline bool is_area() const {
    return type == AREA || type == RECT;
  }

  inline bool is_infinite() const {
    return type == INFINITE;
  }

  /* get the material for this light source */
  inline uint32_t matid() const {
    return details->matid;
  }

  /* constructs a point light source */
  static light_t* make_point(material_t* material, const Imath::V3f& position);

  /* constructs a distant point light source (i.e. sun light) */
  static light_t* make_distant(material_t* material, const Imath::V3f& direction, float angle);

  /* add a simple rectangular area light */
  static light_t* make_rect(material_t* material, const Imath::M44f& transform, float width, float height);

  /* constructs an area light source from a face set of a mesh */
  static light_t* make_area(mesh_t* m, uint32_t set);

  /* constructs an infinite light from a material */
  static light_t* make_infinite(const material_t* material);
};
