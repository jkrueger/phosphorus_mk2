#pragma once

#include "params.hpp"
#include "math/orthogonal_base.hpp"
#include "math/sampling.hpp"
#include "math/vector.hpp"

namespace oren_nayar {
  Imath::Color3f f(
    const bsdf::lobes::oren_nayar_t& params
  , const Imath::V3f& wi
  , const Imath::V3f& wo)
  {
    invertible_base_t base(params.n);

    const auto li = base.to_local(wi);
    const auto lo = base.to_local(wo);
 
    const auto cos_theta_i = std::abs(ts::cos_theta(li));
    const auto cos_theta_o = std::abs(ts::cos_theta(lo));
    const auto sin_theta_i = ts::sin_theta(li);
    const auto sin_theta_o = ts::sin_theta(lo);

    auto max_cos = 0.0f;
    if (sin_theta_i > 0.0001f && sin_theta_o > 0.0001f) {
      const auto sin_phi_i = ts::sin_phi(li), cos_phi_i = ts::cos_phi(li);
      const auto sin_phi_o = ts::sin_phi(lo), cos_phi_o = ts::cos_phi(lo);

      const auto dcos = cos_phi_i * cos_phi_o + sin_phi_i * sin_phi_o;

      max_cos = std::max(0.0f, dcos);
    }

    float sin_alpha, tan_beta;
    if (cos_theta_i > cos_theta_o) {
      sin_alpha = sin_theta_o;
      tan_beta  = sin_theta_i / cos_theta_i;
    }
    else {
      sin_alpha = sin_theta_i;
      tan_beta  = sin_theta_o / cos_theta_o;
    }

    const auto result = (params.a + params.b * max_cos * sin_alpha * tan_beta);

    return Imath::Color3f(result * M_1_PI);
  }

  float pdf(
    const bsdf::lobes::oren_nayar_t& params
  , const Imath::V3f& wi
  , const Imath::V3f& wo)
  {
    return params.n.dot(wi) * M_1_PI;
  }

  Imath::Color3f sample(
    const bsdf::lobes::oren_nayar_t& params
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
};
