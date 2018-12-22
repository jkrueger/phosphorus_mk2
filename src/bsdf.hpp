#pragma once

#include "bsdf/params.hpp"
#include "utils/color.hpp"

/* models a surface reflection function which can be evaluated and 
 * sampled by an integrator */
struct bsdf_t {
  static const uint32_t MaxLobes = 8;

  /* we need a type flag for the bsdf lobes, since this will run
   * on the gpu as well, where we don't have virtual fcuntion calls
   * the type flags are used for dispatch  */
  enum type_t {
    Emissive    = 0,
    Diffuse     = 1,
    OrenNayar   = 2,
    Reflection  = 4,
    Refraction  = 8,
    Microfacet  = 16,
    Background  = 32,
    Transparent = 64,
  };

  union param_t{
    bsdf::lobes::diffuse_t diffuse;
    bsdf::lobes::oren_nayar_t oren_nayar;
    bsdf::lobes::reflect_t reflect;
    bsdf::lobes::refract_t refract;
    bsdf::lobes::microfacet_t microfacet;
  };

  type_t type[MaxLobes];
  Imath::Color3f weight[MaxLobes];
  uint8_t params[MaxLobes*sizeof(param_t)];
  uint32_t lobes;
  uint32_t flags[MaxLobes];

  bsdf_t();

  /* evaluate the bsdf for a given pair of directions */
  Imath::Color3f f(const Imath::V3f& wi, const Imath::V3f& wo) const;

  /* sample the bsdf given an incident direction */
  Imath::Color3f sample(
    const Imath::V2f& sample
  , const Imath::V3f& wi
  , Imath::V3f& wo
  , float& pdf
  , uint32_t& flags) const;

  template<typename T>
  inline void add_lobe(type_t t, const Imath::Color3f& c, const T* p) {
    type[lobes] = t;
    flags[lobes] = T::flags;
    weight[lobes] = c;
    memcpy(&params[lobes*sizeof(param_t)], p, sizeof(T));
    ++lobes;
  }

  template<typename T>
  inline void add_lobe(type_t t, const Imath::Color3f& c, const T& p) {
    type[lobes] = t;
    flags[lobes] = T::flags;
    weight[lobes] = c;
    memcpy(&params[lobes*sizeof(param_t)], &p, sizeof(T));
    ++lobes;
  }

  static bool is_specular(uint32_t flags) {
    return (flags & bsdf::SPECULAR) == bsdf::SPECULAR;
  }
};
