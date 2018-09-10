#pragma once

#include "../film.hpp"
#include "../entities/camera.hpp"

namespace film {
  struct file_t : public film_t<> {
    std::string path;
    uint32_t width, height;
    float* pixels;

    file_t(camera_t::film_t& config, const std::string& path);
    ~file_t();

    void add_tile(
      const Imath::V2i& pos
    , const Imath::V2i& size
    , const color_t* splats);

    void finalize();
  };
}
