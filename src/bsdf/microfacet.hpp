#pragma once

#include "params.hpp"
#include "math/orthogonal_base.hpp"
#include "math/vector.hpp"

#include <ImathColor.h>

namespace microfacet {
  namespace ggx{
    inline float D(const Imath::V3f& v, float ax, float ay) {
      const auto tan2_theta = ts::tan2_theta(v);
      if (std::isinf(tan2_theta)) {
        return 0.0f;
      }

      const auto cos2_theta = ts::cos2_theta(v);
      const auto cos4_theta = cos2_theta*cos2_theta;

      const auto e = (ts::cos2_phi(v) / (ax*ax) +
        ts::sin2_phi(v) / (ay*ay)) * tan2_theta;

      return 1.0f/(M_PI*ax*ay*cos4_theta*(1+e)*(1+e));
    }

    inline float G(const Imath::V3f& v, float ax, float ay) {
      const auto abs_tan_theta = std::abs(ts::tan_theta(v));
      if (std::isinf(abs_tan_theta)) {
        return 0.0f;
      }

      const auto alpha = std::sqrt(
        ts::cos2_phi(v) * ax * ay +
        ts::sin2_phi(v) * ax * ay);

      const auto alpha2_tan2_theta =
        (alpha * abs_tan_theta) * (alpha * abs_tan_theta);

      return (-1.0f + std::sqrt(1.0f + alpha2_tan2_theta)) * 0.5f;
    }

    inline void sample_slope(
      float cos_theta
    , float& slope_x
    , float& slope_y
    , const Imath::V2f& uv)
    {
      auto u = uv.x;
      auto v = uv.y;
      
      if (cos_theta > .9999) {
        float r   = std::sqrt(u / (1 - u));
        float phi = 6.28318530718 * v;
        slope_x = r * std::cos(phi);
        slope_y = r * std::sin(phi);
      }

      const auto sin_theta = std::sqrt(std::max(0.0f, 1.0f - (cos_theta*cos_theta)));
      const auto tan_theta = sin_theta / cos_theta;
      const auto a  = 1.0f / tan_theta;
      const auto g1 = 2.0f / (1.0f + std::sqrt(1.0f + 1.0f / (a*a)));

      const auto A   = 2.0f * u / g1 - 1.0f;
      auto tmp = 1.0f / (A*A - 1.0f);
      if (tmp > 1e10) { tmp = 1e10; }
      const auto B = tan_theta;
      const auto D =
        std::sqrt(std::max((float) B*B*tmp*tmp - (A*A-B*B) * tmp, 0.0f));
      const auto slope_x1 = B*tmp-D;
      const auto slope_x2 = B*tmp+D;

      slope_x = (A < 0.0f || slope_x2 > 1.0f / tan_theta) ? slope_x1 : slope_x2;

      float S;
      if (v > 0.5f) {
        S = 1.0f;
        v = 2.0f * (v - 0.5f);
      }
      else {
        S = -1.0f;
        v = 2.0f * (0.5f - v);
      }
      
      const auto z =
        (v * (v * (v * 0.27385f - 0.73369f) + 0.46341f)) /
        (v * (v * (v * 0.093073f + 0.309420f) - 1.0f) + 0.597999f);

      slope_y = S * z * std::sqrt(1.0f + slope_x * slope_x);
    }

    inline Imath::V3f sample(
      const Imath::V3f& wi
    , float& pdf 
    , const Imath::V2f& uv
    , float alpha_x
    , float alpha_y)
    {
      Imath::V3f stretched(alpha_x * wi.x, wi.y, alpha_y * wi.z);
      stretched.normalize();
      
      float slope_x, slope_y;
      sample_slope(ts::cos_theta(stretched), slope_x, slope_y, uv);

      const auto tmp =
        ts::cos_phi(stretched) * slope_x -
        ts::sin_phi(stretched) * slope_y;

      slope_y =
        ts::sin_phi(stretched) * slope_x +
        ts::cos_phi(stretched) * slope_y;
      slope_x = tmp;

      slope_x = slope_x * alpha_x;
      slope_y = slope_y * alpha_y;

      Imath::V3f wh(-slope_x, 1.0f, -slope_y);
      wh.normalize();

      pdf = D(wh, alpha_x, alpha_y) * ts::cos_theta(wh);

      return wh;
    }
  }

  static inline float roughness_to_alpha(float roughness) {
    roughness = std::max(roughness, (float) 1e-3);
    float x = std::log(roughness);
    return 1.62142f
      + 0.819955f * x
      + 0.1734f * x * x
      + 0.0171201f * x * x * x
      + 0.000640711f * x * x * x * x;
  }

  /* Cook-Torrance microfacet model
   * Source: PBRT */
  template<typename Dt, typename Gt, typename Ft>
  inline Imath::Color3f f(
    const bsdf::lobes::microfacet_t& params
  , const Imath::V3f& wi
  , const Imath::V3f& wo
  , const Dt& D
  , const Gt& G
  , const Ft& F)
  {
    invertible_base_t base(params.n);

    const auto li = base.to_local(wi);
    const auto lo = base.to_local(wo);
    
    auto wh = li + lo;

    const auto cos_ti = std::abs(ts::cos_theta(li));
    const auto cos_to = std::abs(ts::cos_theta(lo));

    if (cos_ti == 0 || cos_to == 0) {
      return Imath::Color3f(0.0f);
    }

    if (wh.x == 0 || wh.y == 0 || wh.z == 0) {
      return Imath::Color3f(0.0f);
    }

    wh.normalize();

    const auto ax = roughness_to_alpha(params.xalpha);
    const auto ay = roughness_to_alpha(params.yalpha);

    const auto cos_h = li.dot(wh);
    const auto cos_h2 = lo.dot(wh);
    const auto g = 1.0f / (1.0f + G(li, ax, ay) + G(lo, ax, ay));
    const auto c = D(wh, ax, ay) * g /* F(cos_h, params.eta) */ *
      (1.0f / (4.0f * cos_ti * cos_to));

    return Imath::Color3f(c);
  }

  float pdf(
    const bsdf::lobes::microfacet_t& params
  , const Imath::V3f& wi
  , const Imath::V3f& wo)
  {
    if (!in_same_hemisphere(wo, wi)) return 0;

    Imath::V3f wh = (wo + wi).normalize();

    const auto ax = roughness_to_alpha(params.xalpha);
    const auto ay = roughness_to_alpha(params.yalpha);

    return ggx::D(wh, ax, ay) / (4.0f * wo.dot(wh));
  }

  template<typename Dt, typename Gt, typename Ft>
  Imath::Color3f sample(
    const bsdf::lobes::microfacet_t& params
  , const Imath::V3f& wi
  , Imath::V3f& wo
  , const Imath::V2f& sample
  , float& pdf
  , const Dt& D
  , const Gt& G
  , const Ft& F)
  {
    const invertible_base_t base(params.n);

    const auto li = base.to_local(wi);

    if (li.y == 0.0f) {
      return Imath::V3f(0.0f);
    }

    const auto ax = roughness_to_alpha(params.xalpha);
    const auto ay = roughness_to_alpha(params.yalpha);

    float dpdf;
    const auto wh = ggx::sample(li, dpdf, sample, ax, ay);

    wo  = base.to_world(-li + (2.0f * li.dot(wh)) * wh);
    pdf = dpdf / (4.0f * li.dot(wh));

    return f(params, wi, wo, D, G, F);
  }
}
