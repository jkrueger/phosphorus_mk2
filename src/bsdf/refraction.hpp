#pragma once

#include "params.hpp"
#include "math/fresnel.hpp"
#include "math/sampling.hpp"

#include <cmath>

namespace refraction {
  Imath::Color3f sample(
    const bsdf::lobes::refract_t& params
  , const Imath::V3f& wi
  , Imath::V3f& wo
  , const Imath::V2f& sample
  , float& pdf)
  {
    pdf = 1.0f;

    auto cos_theta = params.n.dot(wi);
    auto sin_theta = std::max(0.0f, 1.0f - cos_theta * cos_theta);

    Imath::V3f n;
    float eta = params.eta;

    if (cos_theta > 0) {
      n = params.n;
      eta = 1.0f / eta;
    }
    else {
      n = -params.n;
      cos_theta = -cos_theta;
    }

    auto arg = 1.0f - (eta * eta * sin_theta);

    if (arg >= 0.0f) {
      auto dnp = std::sqrt(arg);
      auto nk  = eta * cos_theta - dnp;

      wo = -wi * eta + n * nk;

      const auto out = 1.0f - fresnel::dielectric(cos_theta, params.eta);

      return Imath::Color3f(out);
    }

    return Imath::Color3f();
  }
}
