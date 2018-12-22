#pragma once

#include <math.h>

namespace trig {
  inline constexpr float radians(float a) {
    return a * (M_PI / 180.0f);
  }
}
