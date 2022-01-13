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
    Sheen       = 32,
    Background  = 64,
    Transparent = 128,
  };

  union param_t{
    bsdf::lobes::diffuse_t diffuse;
    bsdf::lobes::oren_nayar_t oren_nayar;
    bsdf::lobes::reflect_t reflect;
    bsdf::lobes::refract_t refract;
    bsdf::lobes::microfacet_t microfacet;
    bsdf::lobes::sheen_t sheen;
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

    const auto index = lobes*sizeof(param_t);
    auto param = (T*) &params[index];

    memcpy(param, p, sizeof(T));
    param->precompute();

    ++lobes;
  }

  template<>
  inline void add_lobe(type_t t, const Imath::Color3f& c, const bsdf::lobes::microfacet_t* p) {
    type[lobes] = t;
    flags[lobes] = p->refract ? bsdf::TRANSMIT : bsdf::REFLECT;
    weight[lobes] = c;

    const auto index = lobes*sizeof(param_t);
    auto param = (bsdf::lobes::microfacet_t*) &params[index];

    memcpy(param, p, sizeof(bsdf::lobes::microfacet_t));
    param->precompute();

    ++lobes;
  }

  bool is_reflective(uint32_t i) const {
    return (flags[i] & bsdf::REFLECT) == bsdf::REFLECT;
  }

  bool is_transmissive(uint32_t i) const {
    return (flags[i] & bsdf::TRANSMIT) == bsdf::TRANSMIT;
  }

  static bool is_specular(uint32_t flags) {
    return (flags & bsdf::SPECULAR) == bsdf::SPECULAR;
  }
};
