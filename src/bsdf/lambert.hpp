#pragma once

#include "params.hpp"

namespace lambert {
  color_t f(const lambert_t& params, const Imath::V3f& wi, Imath::V3f& wo) {
    return 1;
  }

  float pdf(const Imath::V3f& wi, Imath::V3f& wo) {
    return wi * M_1_PI;
  }
}
