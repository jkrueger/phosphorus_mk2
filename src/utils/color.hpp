#pragma once

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-register"
#include <ImathColor.h>
#pragma clang diagnostic pop

namespace color {
  inline bool is_black(const Imath::Color3f& c) {
    return c.x == 0.0f && c.y == 0.0f && c.z == 0.0f;
  }

  inline float y(const Imath::Color3f& c) {
    static const float weight[3] = {0.212671, 0.715160, 0.072169};
    return weight[0] * c.x + weight[1] * c.y + weight[2] * c.z;
  }
}
