#pragma once

#include <Imath/ImathBox.h>

namespace aabb {

  inline bool is_empty_on(const Imath::Box3f& box, uint32_t axis) {
    return box.max[axis] < box.min[axis];
  }

  inline float area(const Imath::Box3f& box) {
    auto d = box.max - box.min;
    return 2.0 * (d.x * d.y + d.x * d.z + d.y * d.z);
  }
}
