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
    Emissive         = 0,
    Diffuse          = 1,
    Translucent      = 2,
    OrenNayar        = 4,
    Reflection       = 8,
    Refraction       = 16,
    Microfacet       = 32,
    Sheen            = 64,
    Background       = 128,
    Transparent      = 256,
    DisneyDiffuse    = 512,
    DisneyRetro      = 1024,
    DisneySheen      = 2048,
    DisneyMicrofacet = 4096
  };

  union param_t{
    bsdf::lobes::diffuse_t diffuse;
    bsdf::lobes::translucent_t translucent;
    bsdf::lobes::oren_nayar_t oren_nayar;
    bsdf::lobes::disney_retro_t disney_retro;
    bsdf::lobes::reflect_t reflect;
    bsdf::lobes::refract_t refract;
    bsdf::lobes::microfacet_t microfacet;
    bsdf::lobes::disney_microfacet_t disney_microfacet;
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

    if (p) {
      const auto index = lobes*sizeof(param_t);
      auto param = (T*) &params[index];

      memcpy(param, p, sizeof(T));
      param->precompute();
    }

    ++lobes;
  }

  inline void add_microfacet_lobe(type_t t, const Imath::Color3f& c, const bsdf::lobes::microfacet_t* p) {
    add_lobe(t, c, p);
    flags[lobes-1] |= p->refract ? bsdf::TRANSMIT : bsdf::REFLECT;
  }

  bool is_reflective(uint32_t i) const {
    return (flags[i] & bsdf::REFLECT) == bsdf::REFLECT;
  }

  bool is_transmissive(uint32_t i) const {
    return (flags[i] & bsdf::TRANSMIT) == bsdf::TRANSMIT;
  }

  static bool is_diffuse(uint32_t flags) {
    return (flags & bsdf::DIFFUSE) == bsdf::DIFFUSE;
  }

  static bool is_specular(uint32_t flags) {
    return (flags & bsdf::SPECULAR) == bsdf::SPECULAR;
  }

  static bool is_glossy(uint32_t flags) {
    return (flags & bsdf::GLOSSY) == bsdf::GLOSSY;
  }
};
