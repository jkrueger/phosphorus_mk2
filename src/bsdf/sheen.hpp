#pragma once

#include "params.hpp"
#include "microfacet.hpp"
#include "math/vector.hpp"

#include <ImathColor.h>
#include <ImathVec.h>

namespace microfacet {
  /* Implements a microfacet reflection modek for velvety materials.
   * Source: Sony Pictures ImageWorks, Production Friendly Microfacet Sheen BRDF */
  namespace sheen {
    namespace details {
      /* lambda term from the ImageWorks paper */
      inline float L(float x, float r) {
        static const float p0[] = { 25.3245f, 3.32435f, 0.16801f, -1.27393f, -4.85967f };
        static const float p1[] = { 21.5473f, 3.82987f, 0.19823f, -1.97760f, -4.32054f };

        const auto interp = [](float a, float b, float t) -> float {
          return t * a + (1.0f - t) * b;
        };

        const auto t = (1.0f - r) * (1.0f - r);

        const auto a = interp(p0[0], p1[0], t);
        const auto b = interp(p0[1], p1[1], t);
        const auto c = interp(p0[2], p1[2], t);
        const auto d = interp(p0[3], p1[3], t);
        const auto e = interp(p0[4], p1[4], t);

        const auto xc = std::pow(x, c);

        return a / (1 + b * xc) + d * x + e;
      }
    }

    struct distribution_t {
      /* micro facet distribution function */
      inline float D(
        const bsdf::lobes::sheen_t& params
      , const Imath::V3f& v) const
      { 
        const auto sin_theta = ts::sin_theta(v);
        const auto oor = 1.0f / params.r;

        return (2.0f + oor) * std::pow(sin_theta, oor) / (2.0f * M_PI);
      }

      /* shadowing term */
      inline float G(
        const bsdf::lobes::sheen_t& params
      , const Imath::V3f& v) const
      {
        using details::L;

        static const auto L5 = L(0.5f, params.r);

        const auto cos_theta = ts::cos_theta(v);
        const auto l = (cos_theta < 0.5f) ?
          L(cos_theta, params.r) : 2.0f * L5 - L(1.0f - cos_theta, params.r);
        
        return std::exp(l);
      }
    };

    float pdf(
      const bsdf::lobes::sheen_t& params
    , const Imath::V3f& wi
    , const Imath::V3f& wo)
    {
      return params.n.dot(wi) * M_1_PI;
    }

    Imath::Color3f sample(
      const bsdf::lobes::sheen_t& params
    , const Imath::V3f& wi
    , Imath::V3f& wo
    , const Imath::V2f& sample
    , float& pdf)
    {
      const orthogonal_base_t base(params.n);

      sample::hemisphere::cosine_weighted(sample, wo, pdf);
      wo = base.to_world(wo);

      return cook_torrance::f(params, wi, wo, distribution_t());
    }
  }
}
