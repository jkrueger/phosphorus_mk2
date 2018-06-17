#pragma once

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-register"
#include <ImathMatrix.h>
#pragma clang diagnostic pop

/* The camera data model, used by the rest of the rendering 
 * system */
struct camera_t {
  Imath::M44f to_world;

  float focal_length;
  float sensor_width;

  struct film_t {
    uint32_t width;
    uint32_t height;

    inline film_t()
      : width(1980)
      , height(1080)
    {}
  } film;

  inline camera_t()
    : focal_length(35.0f)
    , sensor_width(32.0f)
  {}
};
