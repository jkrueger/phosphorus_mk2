#pragma once

#include "params.hpp"
#include "math/fresnel.hpp"
#include "math/sampling.hpp"

namespace reflection {
  Imath::Color3f sample(
    const bsdf::lobes::reflect_t& params
  , const Imath::V3f& wi
  , Imath::V3f& wo
  , const Imath::V2f& sample
  , float& pdf)
  {
    const auto cos_theta = params.n.dot(wi);

    pdf = 1.0f;
    wo = -wi + (2.0f * cos_theta) * params.n;

    return Imath::Color3f(1.0f);
  }
}
