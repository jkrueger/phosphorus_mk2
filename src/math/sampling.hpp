#pragma once

#include <cmath>

namespace sample {
  namespace hemisphere {
    static const float_t UNIFORM_DISC_PDF = 1.0f / M_PI; 

    inline void uniform(
      const Imath::V2f& sample
    , Imath::V3f& out
    , float& pdf)
    {
      const float r   = std::sqrt(1.0 - sample.x * sample.x);
      const float phi = 2 * M_PI * sample.y;

      out = Imath::V3f(cos(phi) * r, sample.x, sin(phi) * r);
      pdf = UNIFORM_DISC_PDF;
    }

    inline void cosine_weighted(
      const Imath::V2f& sample
    , Imath::V3f& out
    , float& pdf)
    {
      const float r = std::sqrt(sample.x);
      const float theta = 2 * M_PI * sample.y;
 
      const float x = r * std::cos(theta);
      const float y = r * std::sin(theta);
 
      out = Imath::V3f(x, std::sqrt(std::max(0.0f, 1.0f - sample.x)), y);
      pdf = out.y * UNIFORM_DISC_PDF;
    }
  }
}
