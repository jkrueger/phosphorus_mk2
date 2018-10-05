#pragma once

#include <cmath>

struct orthogonal_base_t {
  Imath::V3f a, b, c;

  inline orthogonal_base_t()
  {}

  inline orthogonal_base_t(const Imath::V3f& n)
    : a(n.cross(std::abs(n.z) < 0.5 ? Imath::V3f(0.0, 0.0, 1.0) : Imath::V3f(0.0, -1.0, 0.0))),
      b(n),
      c(a.cross(n))
  {
    a.normalize();
    c.normalize();
  }

  inline orthogonal_base_t(const Imath::V3f& z, const Imath::V3f& y)
    : a(z.cross(y)), b(y), c(z)
  {
    b.normalize();
  }

  inline Imath::V3f to_world(const Imath::V3f& v) const {
    return v.x * a + v.y * b + v.z * c;
  }
};

struct invertible_base_t : public orthogonal_base_t {
  Imath::V3f ia, ib, ic;

  inline invertible_base_t()
  {}

  inline invertible_base_t(const Imath::V3f& n)
    : orthogonal_base_t(n),
      // transpose base vectors
      ia(a.x,b.x,c.x),
      ib(a.y,b.y,c.y),
      ic(a.z,b.z,c.z)
  {}

  inline Imath::V3f to_local(const Imath::V3f& v) const {
    return v.x * ia + v.y * ib + v.z * ic;
  }
};