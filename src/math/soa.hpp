#pragma once

#include <ImathVec.h>

struct bsdf_t;

namespace soa {
  template<int N>
  struct alignas(32) vector2_t {
    float x[N];
    float y[N];

    Imath::V2f at(uint32_t i) const {
      return Imath::V2f(x[i], y[i]);
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
      x[i] = v.x;
      y[i] = v.y;
      z[i] = v.z;
    }
  };

  /* a stream of rays */
  template<int N>
  struct ray_t {
    vector3_t<N> p;
    vector3_t<N> wi;
    float d[N];
  };

  /* a stream of shading parameters and results */
  template<int N>
  struct shading_t {
    uint32_t     mesh[N];
    uint32_t     set[N];
    uint32_t     face[N];
    vector3_t<N> n;
    float        u[N];
    float        v[N];
    float        s[N];
    float        t[N];
    vector3_t<N> r;
    bsdf_t*      bsdf[N];
  };
}
