#pragma once

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-register"
#include <ImathColor.h>
#pragma clang diagnostic pop

#include <iostream>

struct color_t {
  union {
    struct {
      float r, g, b;
    };
    float v[3];
  };

  inline color_t()
    : r(0), g(0), b(0)
  {}

  inline color_t(float x)
    : r(x), g(x), b(x)
  {}

  inline color_t(const color_t& cpy)
    : r(cpy.r), g(cpy.g), b(cpy.b)
  {}

  inline color_t(float _r, float _g, float _b)
    : r(_r), g(_g), b(_b)
  {}

  inline color_t(const Imath::Color3f& c)
    : r(c.x), g(c.y), b(c.z)
  {}

  static inline color_t from_rgb(uint8_t r, uint8_t g, uint8_t b) {
    return color_t(r/255.0f, g/255.0f, b/255.0f);
  }

  inline color_t& scale(float s) {
    r *= s; g *= s; b *= s;
    return *this;
  }

  inline float y() const {
    static const float weight[3] = {0.212671, 0.715160, 0.072169};
    return weight[0] * r + weight[1] * g + weight[2] * b;
  }
};

inline color_t operator+(const color_t& l, const color_t& r) {
  color_t out(l);
  out.r += r.r; out.g += r.g; out.b += r.b;
  return out;
}

inline color_t& operator+=(color_t& l, const color_t& r) {
  l.r += r.r; l.g += r.g; l.b += r.b;
  return l;
}

inline color_t operator-(const color_t& l, const color_t& r) {
  color_t out(l);
  out.r -= r.r; out.g -= r.g; out.b -= r.b;
  return out;
}

inline color_t operator*(const color_t& l, float r) {
  color_t out(l);
  out.scale(r);
  return out;
}

inline color_t& operator*=(color_t& l, const color_t& r) {
  l.r *= r.r; l.g *= r.g; l.b *= r.b;
  return l;
}

inline color_t operator*(const color_t& l, const color_t& r) {
  return color_t(l.r*r.r, l.g*r.g, l.b*r.b);
}

inline std::ostream& operator<<(std::ostream& o, const color_t& c) {
  return o << "color_t{" << c.r << ", " << c.g << ", " << c.b << "}";
}
