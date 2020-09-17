#include "file.hpp"

#include "buffer.hpp"

#include <OpenImageIO/imagebuf.h>

using namespace OIIO;

namespace film {
  struct file_t::details_t {
    std::string path;
    ImageBuf image;

    inline details_t(const std::string& path, camera_t::film_t& config)
     : path(path)
     , image(ImageSpec(config.width, config.height, 4, TypeFloat))
    {}
  };

  file_t::file_t(camera_t::film_t& config, const std::string& path)
    : details(new details_t(path, config))
  {}

  file_t::~file_t() {
  }

  void file_t::add_tile(
    const Imath::V2i& pos
  , const Imath::V2i& size
  , const render_buffer_t& buffer)
  {
    const auto channel = buffer.channel(0);
    
    details->image.set_pixels(
      ROI(pos.x, pos.y, size.x, size.y),
      TypeFloat,
      channel->data(), 
      channel->xstride(), 
      channel->ystride()
    );
  }

  void file_t::finalize() {
    details->image.write(details->path);
  }
}
