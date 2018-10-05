#pragma once

#include "params.hpp"
#include "math/sampling.hpp"

namespace lambert {
  color_t f(
    const bsdf::lobes::diffuse_t& params
  , const Imath::V3f& wi
  , const Imath::V3f& wo)
  {
    return M_1_PI;
  }

  float pdf(
    const bsdf::lobes::diffuse_t& params
  , const Imath::V3f& wi
  , const Imath::V3f& wo)
  {
    return params.n.dot(wi) * M_1_PI;
  }

  void sample(
    const bsdf::lobes::diffuse_t& params
  , const Imath::V3f& wi
  , Imath::V3f& wo
  , const Imath::V2f& sample
  , float& pdf)
  {
    sample::hemisphere::cosine_weighted(sample, wo, pdf);
  }
}
