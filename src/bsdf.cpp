#include "bsdf.hpp"
#include "sampling.hpp"

#include "bsdf/lambert.hpp"

#include <cmath>

color_t eval(
  bsdf_t::type_t type
, const bsdf_t::param_t& param
, const Imath::V3f& wi
, const Imath::V3f& wo
, float& pdf)
{
  color_t result;

  switch(type) {
  case bsdf_t::Diffuse:
    {
      pdf    = lambert::pdf(param.diffuse, wi, wo);
      result = lambert::f(param.diffuse, wi, wo);
      break;
    }
  default:
    std::cout << "Can't evaluate BSDF type: " << type << std::endl;
    break;
  }

  return result;
}

color_t bsdf_t::f(const Imath::V3f& wi, const Imath::V3f& wo) const {
  color_t out;
  float   ignored;

  const auto p = (param_t*) params;

  for (auto i=0; i<lobes; ++i) {
    out = eval(type[i], p[i], wi, wo, ignored);
  }

  return out;
}

color_t bsdf_t::sample(
  const Imath::V2f& sample
, const Imath::V3f& wi
, Imath::V3f& wo
, float& pdf) const
{
  auto index = std::min((uint32_t) std::floor(sample.x * lobes), (lobes-1));
  float sum = 0;

  const auto p = (param_t*) params;

  switch(type[index]) {
  case Diffuse:
    {
      lambert::sample(p[index].diffuse, wi, wo, sample, pdf);
      break;
    }
  default:
    // std::cout << "Can't sample BSDF type: " << type[index] << std::endl;
    break;
  }

  color_t result;

  for (auto i=0; i<lobes; ++i) {
    if (i != index) {
      float lobe_pdf;
      result += eval(type[i], p[i], wi, wo, lobe_pdf);
      pdf    += lobe_pdf;
    }
  }

  pdf /= lobes;

  return result;
}
