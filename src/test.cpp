#include "bsdf/microfacet.hpp"

int main() {

  bsdf::lobes::microfacet_t p;
  p.xalpha = 0.001f;
  p.yalpha = 0.001f;
  p.eta = 1.1f;

  p.precompute();

  Imath::V3f wo;
  float pdf;

  const auto f = microfacet::cook_torrance::refract::sample(p, {0.564088, 0.197126, 0.801694}, wo, {0.319097, 0.997709}, pdf, microfacet::ggx_t());

  std::cout << f << " -> " << wo << std::endl;

  return 0;
}
