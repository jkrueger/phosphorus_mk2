#pragma once

#include "utils/color.hpp"

#include <OpenEXR/ImathVec.h>

template<int N = 1024>
struct film_t {
  /* Add radiance sample values to film */
  virtual void add_tile(
    const Imath::V2i& pos
  , const Imath::V2i& size
  , const Imath::Color3f* splats) = 0;
};
