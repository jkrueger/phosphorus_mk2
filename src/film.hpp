#pragma once

#include "utils/color.hpp"

#include <Imath/ImathVec.h>

struct render_buffer_t;

template<int N = 1024>
struct film_t {
  /* Add radiance sample values to film */
  virtual void add_tile(
    const Imath::V2i& pos
  , const Imath::V2i& size
  , const render_buffer_t& buffer) = 0;
};
