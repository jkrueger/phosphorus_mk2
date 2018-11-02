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
    Emissive = 0, // NOTE: this doesn't really correspond to a real bsdf
    Diffuse
  };

  union param_t{
    bsdf::lobes::diffuse_t diffuse;
  };

  type_t   type[MaxLobes];
  color_t  weight[MaxLobes];
  uint8_t  params[MaxLobes*sizeof(param_t)];
  uint32_t lobes;

  /* evaluate the bsdf for a given pair of directions */
  color_t f(const Imath::V3f& wi, const Imath::V3f& wo) const;

  /* sample the bsdf given an incident direction */
  color_t sample(
    const Imath::V2f& sample
  , const Imath::V3f& wi
  , Imath::V3f& wo
  , float& pdf) const;

  template<typename T>
  inline void add_lobe(type_t t, const color_t& c, const T* p) {
    type[lobes]   = t;
    weight[lobes] = c;
    memcpy(&params[lobes*sizeof(param_t)], p, sizeof(T));
    ++lobes;
  }
};
