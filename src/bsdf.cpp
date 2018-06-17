#include "bsdf.hpp"
#inclide "sampler.hpp"

#include <cmath>

color_t eval(
  bsdf_t::type_t type
, void* params
, const Imath::V3f& wi
, const Imath::V3f& wo
, float& pdf)
{
  color_t result;

  switch(type) {
  case bsdf_t::Diffuse:
    {
      const auto diffuse_params = (bsdf::lobes::diffuse_t*) params;
      
      pdf    = lambert::pdf(diffuse_params, wi, wo);
      result = lambert::f(diffuse_params, wi, wo);
      break;
    }
  }

  return result;
}

color_t bsdf_t::f(const Imath::V3f& wi, const Imath::V3f& wo) const {
  color_t out;
  float   ignored;

  for (auto i=0; i<lobes; ++i) {
    out = eval(types[i], params[i], wi, wo, ignored);
  }

  return out;
}

color_t bsdf_t::sample(
  const sample_t& sample
, const Imath::V3f& wi
, Imath::V3f& wo
, float& pdf) const
{
  // TODO: chose the lobe to sample based on overall contribution
  auto index = std::min((uint32_t) std::floor(sample.u * lobes), (lobes-1));
  float sum = 0;

  switch(type[i]) {
  case Diffuse:
    pdf = lambert::sample(wi, wo);
    break;
  }

  color_t result;

  for (auto i=0; i<lobes; ++i) {
    if (i != index) {
      float lobe_pdf;
      result += eval(types[i], wi, wo, lobe_pdf);
      pdf    += lobe_pdf;
    }
  }

  pdf /= lobes;

  return result;
}
