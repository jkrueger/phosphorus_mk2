#pragma once

#include "config.hpp"

#include <OpenEXR/ImathVec.h>

namespace soa {
  template<int N>
  struct alignas(32) vector2_t {
    float x[N];
    float y[N];

    Imath::V2f at(uint32_t i) const {
      return Imath::V2f(x[i], y[i]);
    }

    void from(uint32_t i, const Imath::V2f& v) {
      assert(i >= 0 && i <= N);
      x[i] = v.x;
      y[i] = v.y;
    }
  };

  template<int N>
  struct alignas(32) vector3_t {
    float x[N];
    float y[N];
    float z[N];

    Imath::V3f at(uint32_t i) const {
      return Imath::V3f(x[i], y[i], z[i]);
    }

    void from(uint32_t i, const Imath::V3f& v) {
      assert(i >= 0 && i <= N);
      x[i] = v.x;
      y[i] = v.y;
      z[i] = v.z;
    }
  };
}
