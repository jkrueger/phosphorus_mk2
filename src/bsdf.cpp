#include "bsdf.hpp"
#include "sampling.hpp"

#include "bsdf/lambert.hpp"
#include "bsdf/reflection.hpp"
#include "bsdf/refraction.hpp"

#include <cmath>

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
  case bsdf_t::Reflection:
    pdf = 0.0f;
    break;
  case bsdf_t::Refraction:
    pdf = 0.0f;
    break;
  default:
    std::cout << "Can't evaluate BSDF type: " << type << std::endl;
    break;
  }

  return result;
}

bsdf_t::bsdf_t()
  : flags(0), lobes(0)
{}

Imath::Color3f bsdf_t::f(const Imath::V3f& wi, const Imath::V3f& wo) const {
  Imath::Color3f out(0.0f);
  float ignored;

  const auto p = (param_t*) params;

  for (auto i=0; i<lobes; ++i) {
    out += eval(type[i], p[i], wi, wo, ignored) * weight[i];
  }

  return out;
}

Imath::Color3f bsdf_t::sample(
  const Imath::V2f& sample
, const Imath::V3f& wi
, Imath::V3f& wo
, float& pdf) const
{
  auto index = std::min((uint32_t) std::floor(sample.x * lobes), (lobes-1));

  const auto one_minus_epsilon =
    1.0f-std::numeric_limits<float>::epsilon();
  const auto u =
    std::min(sample.x * lobes - index, one_minus_epsilon);

  Imath::V2f remapped(u, sample.y);
  Imath::Color3f result(0.0f);

  const auto p = (param_t*) params;

  switch(type[index]) {
  case Diffuse:
    result = lambert::sample(p[index].diffuse, wi, wo, remapped, pdf);
    break;
  case Reflection:
    result = reflection::sample(p[index].reflect, wi, wo, remapped, pdf);
    break;
  case Refraction:
    result = refraction::sample(p[index].refract, wi, wo, remapped, pdf);
    break;
  default:
    std::cout << "Can't sample BSDF type: " << type[index] << std::endl;
    break;
  }

  result *= weight[index];

  for (auto i=0; i<lobes; ++i) {
    if (i != index) {
      float lobe_pdf;
      result += eval(type[i], p[i], wi, wo, lobe_pdf) * weight[i];
      pdf    += lobe_pdf;
    }
  }

  pdf /= lobes;

  return result;
}
