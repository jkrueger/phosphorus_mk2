#pragma once

#include "film.hpp"

#include <MEM_guardedalloc.h>
#include <RNA_access.h>
#include <RNA_blender_cpp.h>
#include <RNA_types.h>

struct render_buffer_t;

namespace blender {
  struct sink_t : public film_t<> {
    struct details_t;
    details_t* details;

    sink_t(
      BL::RenderEngine& engine
    , const std::string& view
    , const std::string& layer
    , uint32_t width
    , uint32_t height);

    void add_tile(
      const Imath::V2i& pos
    , const Imath::V2i& size
    , const render_buffer_t& buffer);
  };
}
