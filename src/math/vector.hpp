#pragma once

#include "utils/algo.hpp"

#include <ImathVec.h>

#include <algorithm>
#include <cmath>

inline bool in_same_hemisphere(const Imath::V3f& a, const Imath::V3f& b) {
  return a.dot(b) >= 0.0;
}

inline Imath::V3f offset(
  const Imath::V3f& p
, const Imath::V3f& n
, bool invert = false)
{
  const auto off = invert ? -0.0001 : 0.0001f;
  return p + n * off;
}

/* tagent space vector, and trigonometry functions */
namespace ts {
  inline bool in_same_hemisphere(const Imath::V3f& a, const Imath::V3f& b) {
    return (a.y * b.y) >= 0.0f;
  }
  
  inline float cos2_theta(const Imath::V3f& v) {
    return v.y*v.y;
  }

  inline float cos_theta(const Imath::V3f& v) {
    return v.y;
  }

  inline float sin2_theta(const Imath::V3f& v) {
    return std::max((float) 0, (float)1 - cos2_theta(v));
  }

  inline float sin_theta(const Imath::V3f& v) {
    return std::sqrt(sin2_theta(v));
  }

  inline float tan_theta(const Imath::V3f& v) {
    return sin_theta(v) / cos_theta(v);
  }

  inline float tan2_theta(const Imath::V3f& v) {
    return sin2_theta(v) / cos2_theta(v);
  }

  inline float cos_phi(const Imath::V3f& v) {
    auto sin_t = sin_theta(v);
    return (sin_t == 0) ? 1 : clamp(v.x / sin_t, -1.f, 1.f);  
  }

  inline float cos2_phi(const Imath::V3f& v) {
    const auto x = cos_phi(v);
    return x*x;
  }

  inline float sin_phi(const Imath::V3f& v) {
    float sin_t = sin_theta(v);
    return (sin_t == 0) ? 0 : clamp(v.z / sin_t, -1.f, 1.f);
  }

  inline float sin2_phi(const Imath::V3f& v) {
    const auto x = sin_phi(v);
    return x*x;
  }
}
