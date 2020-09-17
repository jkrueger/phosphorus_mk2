#pragma once

#include "../film.hpp"
#include "../entities/camera.hpp"

namespace film {
  struct file_t : public film_t<> {
    struct details_t; 
    std::unique_ptr<details_t> details;

    file_t(camera_t::film_t& config, const std::string& path);
    ~file_t();

    void add_tile(
      const Imath::V2i& pos
    , const Imath::V2i& size
    , const render_buffer_t& buffer);

    void finalize();
  };
}
