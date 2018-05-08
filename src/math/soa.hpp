#pragma once

namespace soa {
  template<int N>
  struct alignas(32) vector2_t {
    float x[N];
    float y[N];
  };
  
  template<int N>
  struct alignas(32) vector3_t {
    float x[N];
    float y[N];
    float z[N];
  };

  /* a stream of rays */
  template<int N>
  struct ray_t {
    vector3_t<N> p;
    vector3_t<N> wi;
    float d[N];
  };
}
