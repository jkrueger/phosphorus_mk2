#pragma once

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-register"
#include <ImathMatrix.h>
#pragma clang diagnostic pop

struct camera_t {
  Imath::M44f to_world;

  float focal_length;
  float sensor_width;

  inline camera_t()
    : focal_length(35.0f)
    , sensor_width(32.0f)
  {}
};
