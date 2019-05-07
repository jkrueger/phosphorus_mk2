#pragma once

#include "config.hpp"
#include "simd.hpp"
#include "utils/assert.hpp"

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

    inline Imath::V3f at(uint32_t i) const {
      return Imath::V3f(x[i], y[i], z[i]);
    }

    inline void from(uint32_t i, const Imath::V3f& v) {
      assert(i >= 0 && i <= N);
      x[i] = v.x;
      y[i] = v.y;
      z[i] = v.z;
    }

    inline simd::vector3v_t v_at(uint32_t i) const {
      return simd::vector3v_t(x[i], y[i], z[i]);
    }

    inline simd::vector3v_t stream() const {
      return simd::vector3v_t(x, y, z);
    }

    inline simd::vector3v_t stream(uint32_t off) const {
      return simd::vector3v_t(x + off, y + off, z + off);
    }

    template<int M>
    inline simd::vector3v_t gather(const simd::int32_t<M>& indices) const {
      return simd::vector3v_t(x, y, z, indices);
    }

    inline void from(const simd::vector3v_t& v) {
      v.store(x, y, z);
    }

    inline void from(const simd::vector3v_t& v, uint32_t i) {
      v.store(x + i, y + i, z + i);
    }
  };
}
