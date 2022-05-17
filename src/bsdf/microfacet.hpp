#pragma once

#include "params.hpp"
#include "math/fresnel.hpp"
#include "math/orthogonal_base.hpp"
#include "math/vector.hpp"

#include <Imath/ImathColor.h>

namespace microfacet {

  /* Cook-Torrance microfacet model, Source: PBRT */
  namespace cook_torrance {

    namespace details {
      /* compute shadowing term of the microfacet distribution. 'wi', and 'wo' 
       * are expected to be in tangent space */
      template<typename Params, typename Distribution>
      float G(
          const Params& params
        , const Imath::V3f& wi
        , const Imath::V3f& wo
        , const Distribution& distribution) {
        return 1.0f / (1.0f + distribution.Lambda(params, wi) + distribution.Lambda(params, wo));
      }

      template<typename Params, typename Distribution>
      float G1(
          const Params& params
        , const Imath::V3f& v
        , const Distribution& distribution) {
        return 1.0f / (1.0f + distribution.Lambda(params, v));
      }
    }

    namespace refract {
      template<typename Params, typename Distribution>
      inline Imath::Color3f f(
        const Params& params
      , const Imath::V3f& wi
      , const Imath::V3f& wo
      , const Distribution& distribution) 
      {
        using namespace details;

        invertible_base_t base(params.n);

        const auto li = base.to_local(wi);
        const auto lo = base.to_local(wo);

        if (ts::in_same_hemisphere(li, lo)) {
          return Imath::V3f(0.0f);
        }

        const auto eta = li.y > 0.0f ? params.eta : 1.0f / params.eta;

        // std::cout << "ETA: " << eta << std::endl;

        const auto cos_ti = ts::cos_theta(li);
        const auto cos_to = ts::cos_theta(lo);

        if (cos_ti == 0.0f || cos_to == 0.0f) {
          return Imath::Color3f(0.0f);
        }

        auto wh = (li + lo * eta).normalize();

        if (wh.y < 0) wh = -wh;

        if (lo.dot(wh) * li.dot(wh) > 0) {
          return Imath::Color3f(0.0f);
        }

        const auto f = fresnel::dielectric(lo.dot(wh), eta);

        const auto sqrt_denom = li.dot(wh) + eta * lo.dot(wh);
        const auto factor = 1.0f / eta;
        const auto d = distribution.D(params, wh);
        const auto g = G(params, li, lo, distribution);

        // std::cout << "F: " << f << std::endl;
        // std::cout << "D: " << d << std::endl;
        // std::cout << "G: " << g << std::endl;

        const auto c = (1.0f - f) * std::abs(
            d * g * eta * eta
          * std::abs(lo.dot(wh)) * std::abs(li.dot(wh))
          * factor * factor 
          / (cos_ti * cos_to * sqrt_denom * sqrt_denom));

        return Imath::Color3f(c);
      }

      template<typename Params, typename Distribution>
      float pdf(
        const Params& params
      , const Imath::V3f& wi
      , const Imath::V3f& wo
      , const Distribution& distribution)
      {
        const invertible_base_t base(params.n);

        const auto li = base.to_local(wi);
        const auto lo = base.to_local(wo);

        const auto eta = li.y > 0.0f ? params.eta : 1.0f / params.eta;

        if (in_same_hemisphere(wo, wi)) return 0;

        Imath::V3f wh = (li + lo * eta).normalize();

        const auto sqrt_denom = li.dot(wh) + eta * lo.dot(wh);
        const auto dwh_dwi = std::abs(eta * eta * lo.dot(wh)) / sqrt_denom * sqrt_denom;

        return (distribution.D(params, wh) * ts::cos_theta(wh)) * dwh_dwi;
      }

      template<typename Params, typename Distribution>
      Imath::Color3f sample(
        const Params& params
      , const Imath::V3f& wi
      , Imath::V3f& wo
      , const Imath::V2f& sample
      , float& pdf
      , const Distribution& distribution)
      {
        if (params.eta == 1.0f) {
          // when eta == 1, the surface normal isn't relevant anymore
          // simply return the mirrored vector
          wo  = -wi;
          pdf = 1.0f;
          
          return Imath::V3f(1.0f);
        }

        const invertible_base_t base(params.n);

        const auto li = base.to_local(wi);

        if (li.y == 0.0f) {
          return Imath::V3f(0.0f);
        }

        float dpdf;
        const auto wh = distribution.sample(params, li, dpdf, sample);

        if (wh.dot(li) < 0.0f) {
          return Imath::V3f(0.0f);
        }

        const auto eta = li.y > 0.0f ?  1.0f / params.eta : params.eta;

        const auto cos_ti  = wh.dot(li);
        const auto sin2_ti = std::max(0.0f, 1.0f - cos_ti * cos_ti);
        const auto sin2_tt = eta * eta * sin2_ti;

        if (sin2_tt >= 1.0f) {
          return Imath::Color3f(0.0f);
        }

        const auto cos_tt = std::sqrt(1.0f - sin2_tt);
        const auto lo = eta * -li + (eta * cos_ti - cos_tt) * wh;

        const auto sqrt_denom = li.dot(wh) + eta * lo.dot(wh);
        const auto dwh_dwi = std::abs((eta * eta * lo.dot(wh)) / (sqrt_denom * sqrt_denom));

        pdf = dpdf * dwh_dwi;
        wo = base.to_world(lo);

        return f(params, wi, wo, distribution);
      }
    }

    template<typename Params, typename Distribution>
    inline Imath::Color3f f(
      const Params& params
    , const Imath::V3f& wi
    , const Imath::V3f& wo
    , const Distribution& distribution)
    {
      using namespace details;

      invertible_base_t base(params.n);

      const auto li = base.to_local(wi);
      const auto lo = base.to_local(wo);

      if (!ts::in_same_hemisphere(li, lo)) {
        return Imath::Color3f(0.0f);
      }

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

      const auto d = distribution.D(params, wh);
      const auto g = G(params, li, lo, distribution);
      const auto f = fresnel::dielectric(lo.dot(wh.dot({0.0f, 1.0, 0.0}) < 0.0f ? -wh : wh), 0.5f);

      const auto c = d * g * f * (1.0f / (4.0f * cos_ti * cos_to));

      return Imath::Color3f(c);
    }

    template<typename Params, typename Distribution>
    float pdf(
      const Params& params
    , const Imath::V3f& wi
    , const Imath::V3f& wo
    , const Distribution& distribution)
    {
      const invertible_base_t base(params.n);

      const auto li = base.to_local(wi);
      const auto lo = base.to_local(wo);

      if (!ts::in_same_hemisphere(li, lo)) {
        return 0;
      }

      Imath::V3f wh = (li + lo).normalize();

      return (distribution.D(params, wh) * cook_torrance::details::G1(params, wi, distribution) * std::abs(li.dot(wh)) / std::abs(ts::cos_theta(li))) / (4.0f * li.dot(wh));
    }

    template<typename Params, typename Distribution>
    Imath::Color3f sample(
      const Params& params
    , const Imath::V3f& wi
    , Imath::V3f& wo
    , const Imath::V2f& sample
    , float& opdf
    , const Distribution& distribution)
    {
      const invertible_base_t base(params.n);

      const auto li = base.to_local(wi);

      if (li.y == 0.0f) {
        return Imath::V3f(0.0f);
      }

      float dpdf;
      const auto wh = distribution.sample(params, li, dpdf, sample);
      
      if (li.dot(wh) < 0.0f) {
        return Imath::V3f(0.0f);
      }

      const auto lo  = -li + (2.0f * li.dot(wh)) * wh;

      if (!ts::in_same_hemisphere(li, lo)) {
        return Imath::V3f(0.0f);
      }

      opdf = dpdf / (4.0f * li.dot(wh));
      wo   = base.to_world(lo);

      // if (std::abs(opdf - microfacet::cook_torrance::pdf(params, wi, wo, distribution)) > 0.001) {
      //   std::stringstream ss;
      //   ss << "PDF function is broken: " << opdf << " != " << microfacet::cook_torrance::pdf(params, wi, wo, distribution) << std::endl;
      //   std::cout << ss.str();
      // }

      return f(params, wi, wo, distribution);
    }
  }

  /* Beckman microfacet distribution implementation */
  /*
  struct beckman_t {
    inline float D(
      const bsdf::lobes::microfacet_t& params
    , const Imath::V3f& v) const
    {
    }

    inline float Lambda(
      const bsdf::lobes::microfacet_t& params
    , const Imath::V3f& v) const
    {
    }

    inline Imath::V3f sample(
      const bsdf::lobes::microfacet_t& params
    , const Imath::V3f& wi
    , float& pdf 
    , const Imath::V2f& uv) const
    {
    }
  };
  */

  /* GGX microfacet distribution implementation */
  struct ggx_t {
    /* microfacet distriubtion function */
    inline float D(
      const bsdf::lobes::microfacet_t& params
    , const Imath::V3f& v) const
    {
      const auto tan2_theta = ts::tan2_theta(v);

      if (std::isinf(tan2_theta)) {
        return 0.0f;
      }

      const auto ax = params.xalpha;
      const auto ay = params.yalpha;

      const auto cos2_theta = ts::cos2_theta(v);
      const auto cos4_theta = cos2_theta*cos2_theta;

      const auto e = (ts::cos2_phi(v) / (ax*ax) + ts::sin2_phi(v) / (ay*ay)) * tan2_theta;

      return 1.0f/(M_PI*ax*ay*cos4_theta*(1+e)*(1+e));
    }

    /* shadowing term function */
    inline float Lambda(
      const bsdf::lobes::microfacet_t& params
    , const Imath::V3f& v) const
    {
      const auto abs_tan_theta = std::abs(ts::tan_theta(v));
      if (std::isinf(abs_tan_theta)) {
        return 0.0f;
      }

      const auto ax = params.xalpha;
      const auto ay = params.yalpha;

      const auto alpha = std::sqrt(
        ts::cos2_phi(v) * ax * ay +
        ts::sin2_phi(v) * ax * ay);

      const auto alpha2_tan2_theta = (alpha * abs_tan_theta) * (alpha * abs_tan_theta);

      return (-1.0f + std::sqrt(1.0f + alpha2_tan2_theta)) * 0.5f;
    }

    inline void sample_slope(
      float cos_theta
    , float& slope_x
    , float& slope_y
    , const Imath::V2f& uv) const
    {
      auto u = uv.x;
      auto v = uv.y;
      
      if (cos_theta > .9999) {
        float r   = std::sqrt(u / (1 - u));
        float phi = 6.28318530718 * v;
        slope_x = r * std::cos(phi);
        slope_y = r * std::sin(phi);
        return;
      }

      const auto sin_theta = std::sqrt(std::max(0.0f, 1.0f - (cos_theta*cos_theta)));
      const auto tan_theta = sin_theta / cos_theta;
      const auto a  = 1.0f / tan_theta;
      const auto g1 = 2.0f / (1.0f + std::sqrt(1.0f + 1.0f / (a*a)));

      const auto A   = 2.0f * u / g1 - 1.0f;
      auto tmp = 1.0f / (A*A - 1.0f);
      if (tmp > 1e10) { tmp = 1e10; }
      const auto B = tan_theta;
      const auto D = std::sqrt(std::max((float) B*B*tmp*tmp - (A*A-B*B) * tmp, 0.0f));
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
      const bsdf::lobes::microfacet_t& params
    , const Imath::V3f& wi
    , float& pdf 
    , const Imath::V2f& uv) const
    {
      const auto ax = params.xalpha;
      const auto ay = params.yalpha;
      
      Imath::V3f stretched(ax * wi.x, wi.y, ay * wi.z);
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

      slope_x = slope_x * ax;
      slope_y = slope_y * ay;

      Imath::V3f wh(-slope_x, 1.0f, -slope_y);

      wh.normalize();

      pdf = (D(params, wh) * cook_torrance::details::G1(params, wi, *this) * std::abs(wi.dot(wh)) / std::abs(ts::cos_theta(wi)));

      return wh;
    }
  };
}
