#pragma once

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-register"
#include <Imath/ImathMatrix.h>
#pragma clang diagnostic pop

/* The camera data model, used by the rest of the rendering 
 * system */
struct camera_t {
  Imath::M44f to_world;

  float fov;
  float focal_length;
  float focal_distance;
  float sensor_width;
  float sensor_height;
  float aperture_radius;

  struct film_t {
    uint32_t width;
    uint32_t height;

    inline film_t()
      : width(1280)
      , height(720)
    {}
  } film;

  inline camera_t()
    : focal_length(35.0f)
    , sensor_width(36.0f)
    , sensor_height(24.0f)
    , aperture_radius(0.0f)
  {}

  inline bool is_pinhole() const {
    return aperture_radius == 0.0f;
  }
};
