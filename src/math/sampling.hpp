#pragma once

#include <Imath/ImathVec.h>

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

  namespace disc {
    inline Imath::V2f concentric(const Imath::V2f& sample) {
      static const float pi_o_2 = 2.0f / M_PI;
      static const float pi_o_4 = 4.0f / M_PI;

      const auto offset = 2.0f * sample - Imath::V2f(1, 1);

      float r, theta;
      if (std::abs(sample.x) > std::abs(sample.y)) {
        r = sample.x;
        theta = pi_o_4 * (sample.y / sample.x);
      }
      else {
        r = sample.y;
        theta = pi_o_2 - pi_o_4 * (sample.x / sample.y);
      }

      return r * Imath::V2f(std::cos(theta), std::sin(theta));
    }
  }

  /**
   * Generate a two dimentsional stratified sampling pattern given
   * a random number generator
   */
  template<typename Rng>
  inline void stratified_2d(const Rng& rng, Imath::V2f* out, uint32_t num) {
    const float_t step = 1.0f / (float_t)num;
    float_t dy = 0.0;
    for (auto i=0; i<num; ++i, dy += step) {
      float_t dx = 0.0;
      for (auto j=0; j<num; ++j, dx += step) {
       out[j * num + i] = {
         dx + rng() * step,
         dy + rng() * step
       };
     }
   }
 }
}
