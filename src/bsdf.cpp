#include "bsdf.hpp"
#include "sampling.hpp"
#include "state.hpp"

#include "bsdf/lambert.hpp"
#include "bsdf/disney.hpp"
#include "bsdf/oren_nayar.hpp"
#include "bsdf/reflection.hpp"
#include "bsdf/refraction.hpp"
#include "bsdf/microfacet.hpp"
#include "bsdf/sheen.hpp"

#include "math/fresnel.hpp"

#include <cmath>

namespace ct = microfacet::cook_torrance;

const OIIO::ustring bsdf::lobes::microfacet_t::GGX("ggx");
const OIIO::ustring bsdf::lobes::microfacet_t::BECKMANN("beckmann");

/* the incoming light depends on the normal. the final shading normal is only available
 * after shading, and exists, at least technically, per lobe of the bsdf. hence we need to 
 * modulate the incoming light per lobe, inside the bsdf, instead of somewhere further up
 * in the renderer logic */
inline float angle_to_light(const bsdf_t::param_t& param, const Imath::V3f& wi) {
  return ((const bsdf::lobe_t*) &param)->n.dot(wi);
}

Imath::Color3f eval(
  bsdf_t::type_t type
, const bsdf_t::param_t& param
, const Imath::V3f& wi
, const Imath::V3f& wo
, float& pdf)
{
  Imath::Color3f result(0.0f);

  switch(type) {
  case bsdf_t::Diffuse:
    {
      pdf = lambert::pdf(param.diffuse, wi, wo);
      result = lambert::f(param.diffuse, wi, wo);
      break;
    }
  case bsdf_t::Translucent:
    {
      pdf = lambert::refract::pdf(param.translucent, wi, wo);
      result = lambert::refract::f(param.translucent, wi, wo);
      break;
    }
  case bsdf_t::DisneyDiffuse:
    {
      pdf = lambert::pdf(param.diffuse, wi, wo);
      result = disney::diffuse::f(param.diffuse, wi, wo);
      break;
    }
  case bsdf_t::DisneySheen:
    {
      pdf = lambert::pdf(param.diffuse, wi, wo);
      result = disney::sheen::f(param.diffuse, wi, wo);
      break;
    }
  case bsdf_t::OrenNayar:
    {
      pdf = oren_nayar::pdf(param.oren_nayar, wi, wo);
      result = oren_nayar::f(param.oren_nayar, wi, wo);
      break;
    }
  case bsdf_t::DisneyRetro:
    {
      pdf = lambert::pdf(param.diffuse, wi, wo);
      result = disney::retro::f(param.disney_retro, wi, wo);
      break;
    }
  case bsdf_t::Microfacet:
    {
      if (param.microfacet.is_ggx() || 
          // NOTE: default to ggx as well, until we have a beckman implementation
          param.microfacet.is_beckmann()) {
        if (unlikely(param.microfacet.refract)) {
          pdf = ct::refract::pdf(param.microfacet, wi, wo, microfacet::ggx_t());
          result = ct::refract::f(
            param.microfacet
          , wi
          , wo
          , microfacet::ggx_t());
        }
        else {
          pdf = ct::pdf(param.microfacet, wi, wo, microfacet::ggx_t());
          result = ct::f(
            param.microfacet
          , wi
          , wo
          , microfacet::ggx_t());
        }
      }
      else {
        std::cerr
          << "Unsupported distribution type: "
          << param.microfacet.distribution
          << std::endl;
      }
      break;
    }
  case bsdf_t::DisneyMicrofacet:
    {
      pdf = ct::pdf(param.disney_microfacet, wi, wo, disney::microfacet::disney_ggx_t());
      result = ct::f(
          param.disney_microfacet
        , wi
        , wo
        , disney::microfacet::disney_ggx_t()
        , disney::microfacet::disney_fresnel_t{param.disney_microfacet.metallic, param.disney_microfacet.cspec0}
        );
      // std::cout << "EVAL result: " << result << " pdf: " << pdf << std::endl;
      break;
    }
  case bsdf_t::Sheen:
    {
      pdf = microfacet::sheen::pdf(param.sheen, wi, wo);
      result = ct::f(
        param.sheen
      , wi
      , wo
      , microfacet::sheen::distribution_t());

      break;
    }
  case bsdf_t::Reflection:
    pdf = 0.0f;
    break;
  case bsdf_t::Refraction:
    pdf = 0.0f;
    break;
  case bsdf_t::Transparent:
    pdf = 0.0f;
    break;
  // case bsdf_t::Emissive:
  //   pdf = 1.0f;
  //   result = Imath::Color3f(1.0f);
  //   break;
  default:
    std::cerr << "Can't evaluate BSDF type: " << type << std::endl;
    break;
  }

  return result;
}

bsdf_t::bsdf_t()
  : lobes(0)
{}

Imath::Color3f bsdf_t::f(const Imath::V3f& wi, const Imath::V3f& wo) const {
  Imath::Color3f out(0.0f);
  float ignored;

  param_t* p = ((param_t*) params);

  for (auto i=0; i<lobes; ++i) {
    const auto e = eval(type[i], p[i], wi, wo, ignored);

    const auto atl = angle_to_light(p[i], wi);
    const auto reflect = atl * angle_to_light(p[i], wo) > 0.0f;

    if (type[i] == Emissive || (reflect && is_reflective(i)) || (!reflect && is_transmissive(i))) {
      out += e * weight[i] * atl;
    }
  }

  return out;
}

Imath::Color3f bsdf_t::sample(
  const Imath::V2f& sample
, const Imath::V3f& wi
, Imath::V3f& wo
, float& pdf
, uint32_t& sample_flags) const
{
  auto index = std::min((uint32_t) std::floor(sample.x * lobes), (lobes-1));

  const auto one_minus_epsilon =
    1.0f-std::numeric_limits<float>::epsilon();
  const auto u =
    std::min(sample.x * lobes - index, one_minus_epsilon);

  Imath::V2f remapped(u, sample.y);
  Imath::Color3f result(0.0f);

  const auto& p = ((param_t*) params)[index];

  switch(type[index]) {
  case Diffuse:
  case DisneyDiffuse:
  case DisneySheen:
  case DisneyRetro:
    result = lambert::sample(p.diffuse, wi, wo, remapped, pdf);
    break;
  case OrenNayar:
    result = oren_nayar::sample(p.oren_nayar, wi, wo, remapped, pdf);
    break;
  case Translucent:
    result = lambert::refract::sample(p.translucent, wi, wo, remapped, pdf);
    break;  
  case Microfacet:
    if (p.microfacet.is_ggx() ||
        // NOTE: default to ggx as well, until we have a beckman implementation
        p.microfacet.is_beckmann()) {
      if (unlikely(p.microfacet.refract)) {
        result = ct::refract::sample(
          p.microfacet
        , wi
        , wo
        , remapped
        , pdf
        , microfacet::ggx_t());
      }
      else {
        result = ct::sample(
          p.microfacet
        , wi
        , wo
        , remapped
        , pdf
        , microfacet::ggx_t());
      }
    }
    else {
      std::cerr
        << "Unsupported distribution type: "
        << p.microfacet.distribution
        << std::endl;
    }
    break;
  case DisneyMicrofacet:
    result = ct::sample(
          p.disney_microfacet
        , wi
        , wo
        , remapped
        , pdf
        , disney::microfacet::disney_ggx_t()
        , disney::microfacet::disney_fresnel_t{p.disney_microfacet.metallic, p.disney_microfacet.cspec0});
    // std::cout << "SAMPLE result: " << result << " pdf: " << pdf << std::endl;
    break;
  case Sheen:
    result = microfacet::sheen::sample(p.sheen, wi, wo, remapped, pdf);
    break;
  case Reflection:
    result = reflection::sample(p.reflect, wi, wo, remapped, pdf);
    break;
  case Refraction:
    result = refraction::sample(p.refract, wi, wo, remapped, pdf);
    break;
  case Transparent:
    // wi points away from surface (i.e. the original ray direction, so reverse wi here)
    wo = -wi;
    pdf = 1.0f;
    result = Imath::Color3f(1.0f);
    break;
  default:
    // std::cerr << "Can't sample BSDF type: " << type[index] << std::endl;
    break;
  }

  if (pdf == 0.0f) {
    return Imath::Color3f(0.0f);
  }

  result *= weight[index];

  auto matched_lobes = 1;

  for (auto i=0; i<lobes; ++i) {
    if (i != index && ((flags[index] & flags[i]) == flags[i])) {
      const auto reflect = 
        angle_to_light(((param_t*) params)[i], wi) * 
        angle_to_light(((param_t*) params)[i], wo) > 0.0f;
      if ((reflect && is_reflective(i)) || (!reflect && is_transmissive(i))) {
        auto lobe_pdf = 0.0f;

        result += eval(type[i], ((param_t*) params)[i], wi, wo, lobe_pdf) * weight[i];
        pdf    += lobe_pdf;

        ++matched_lobes;
      }
    }
  }

  pdf /= matched_lobes;
  sample_flags = flags[index];

  return result;
}
