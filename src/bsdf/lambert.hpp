#pragma once

#include "params.hpp"
#include "math/orthogonal_base.hpp"
#include "math/sampling.hpp"
#include "math/vector.hpp"

namespace lambert {
  Imath::Color3f f(
    const bsdf::lobes::diffuse_t& params
  , const Imath::V3f& wi
  , const Imath::V3f& wo)
  {
    return Imath::Color3f(M_1_PI);
  }

  float pdf(
    const bsdf::lobes::diffuse_t& params
  , const Imath::V3f& wi
  , const Imath::V3f& wo)
  {
    return params.n.dot(wi) * M_1_PI;
  }

  Imath::Color3f sample(
    const bsdf::lobes::diffuse_t& params
  , const Imath::V3f& wi
  , Imath::V3f& wo
  , const Imath::V2f& sample
  , float& pdf)
  {
    const orthogonal_base_t base(params.n);

    sample::hemisphere::cosine_weighted(sample, wo, pdf);
    wo = base.to_world(wo);
    return f(params, wi, wo);
  }
  
  namespace refract {
    Imath::Color3f f(
      const bsdf::lobes::translucent_t& params
    , const Imath::V3f& wi
    , const Imath::V3f& wo)
    {
      return Imath::Color3f(M_1_PI);
    }

    float pdf(
      const bsdf::lobes::translucent_t& params
    , const Imath::V3f& wi
    , const Imath::V3f& wo)
    {
      return !in_same_hemisphere(wi, wo) ? 
        std::fabs(params.n.dot(wi)) * M_1_PI : 0.0f;
    }

    Imath::Color3f sample(
      const bsdf::lobes::translucent_t& params
    , const Imath::V3f& wi
    , Imath::V3f& wo
    , const Imath::V2f& sample
    , float& pdf)
    {
      const orthogonal_base_t base(params.n);

      sample::hemisphere::cosine_weighted(sample, wo, pdf);

      if (wo.y > 0) {
        wo.y = -wo.y;
      }

      wo = base.to_world(wo);

      return f(params, wi, wo);
    }
  }
}
