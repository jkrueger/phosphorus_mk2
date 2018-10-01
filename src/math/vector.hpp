#pragma once

#include <OpenEXR/ImathVec.h>

inline bool in_same_hemisphere(const Imath::V3f& a, const Imath::V3f& b) {
  return dot(a, b) > 0.0;
}

inline Imath::V3f offset(const Imath::V3f& p, const Imath::V3f& n) {
  float_t offt = 0.0001f;
  if (!in_same_hemisphere(p, n)) {
    off = -off;
  }
  return p + n * off;
}
