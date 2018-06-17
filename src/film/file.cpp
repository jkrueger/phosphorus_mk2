#include "file.hpp"

#include <OpenImageIO/imageio.h>
using namespace OIIO;

namespace film {
  file_t::file_t(camera_t::film_t& config, const std::string& path)
    : path(path)
    , width(config.width)
    , height(config.height)
    , pixels(new float[config.width*config.height*3])
  {
    memset(pixels, 0, width*height*sizeof(float)*3);
  }

  file_t::~file_t() {
  }

  void file_t::add_tile(
    const Imath::V2i& pos
  , const Imath::V2i& size
  , const color_t* splats)
  {
    int i=0;

    for (auto y=pos.y; y<pos.y+size.y; ++y) {
      for (auto x=pos.x; x<pos.x+size.x; ++x, ++i) {
	const auto index = (y * width + x) * 3;
	pixels[index  ] = splats[i].r;
	pixels[index+1] = splats[i].g;
	pixels[index+2] = splats[i].b;
      }
    }
  }

  void file_t::finalize() {
    ImageOutput* out = ImageOutput::create(path.c_str());
    if (!out) {
      throw std::runtime_error("Unkown image format for output file: " + path);
    }

    ImageSpec spec(width, height, 3, TypeDesc::FLOAT);
    out->open(path.c_str(), spec);
    out->write_image(TypeDesc::FLOAT, pixels);
    out->close();
    ImageOutput::destroy(out);
  }
}
