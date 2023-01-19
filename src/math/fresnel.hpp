#pragma once

#include <cmath>

namespace fresnel {
  inline float dielectric(float cosi, float eta) {
    if (eta == 0) {
      return 1;
    }

    if (cosi < 0.0f) {
      eta = 1.0f / eta;
    }

    float c = std::fabs(cosi);
    float g = eta * eta - 1.0f + c * c;

    if (g > 0.0f) {
      g = std::sqrt(g);

      float A = (g - c) / (g + c);
      float B = (c * (g + c) - 1.0f) / (c * (g - c) + 1.0f);

      return 0.5f * A * A * (1 + B * B);
    }

    return 1.0f;
  }

  inline float schlick_weight(float cos_theta) {
    const auto m = std::clamp(1.0f - cos_theta, 0.0f, 1.0f);
    return (m * m) * (m * m) * m;
  }

  inline Imath::Color3f schlick(const Imath::Color3f& c, float cosi) {
    float weight = schlick_weight(cosi);
    return (1.0f - weight) * c +  weight * Imath::Color3f(1.0f);
  }
}
