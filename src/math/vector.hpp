#pragma once

#include <OpenEXR/ImathVec.h>

inline bool in_same_hemisphere(const Imath::V3f& a, const Imath::V3f& b) {
  return a.dot(b) > 0.0;
}

inline Imath::V3f offset(
  const Imath::V3f& p
, const Imath::V3f& n
, bool invert = false)
{
  const auto off = invert ? -0.0001 : 0.0001f;
  return p + n * off;
}
