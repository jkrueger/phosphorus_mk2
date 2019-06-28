#include "sink.hpp"

#include <mutex>
#include <string>
#include <vector>

namespace blender {
  struct sink_t::details_t {
    BL::RenderEngine engine;

    std::string view;
    std::string layer;

    uint32_t width;
    uint32_t height;

    std::mutex m;

    details_t(
      BL::RenderEngine& engine
    , const std::string& view
    , const std::string& layer
    , uint32_t width
    , uint32_t height)
      : engine(engine)
      , view(view)
      , layer(layer)
      , width(width)
      , height(height)
    {}

    BL::RenderResult begin(const Imath::V2i& pos, const Imath::V2i& size) {
      // blender's (0,0) is at the bottom of the screen, while our's
      // is at the top
      const auto inv_y = height - size.y - pos.y;

      return engine.begin_result(
        pos.x
      , inv_y
      , size.x
      , size.y
      , layer.c_str()
      , view.c_str());
    }

    void pass(BL::RenderResult& result, BL::RenderLayer& layer, float* pixels) {
      auto combined_pass = layer.passes.find_by_name("Combined", view.c_str());
      combined_pass.rect(pixels);
      engine.update_result(result);
    }

    void send(BL::RenderResult result) {
      engine.end_result(result, 0, 0, true);
    }
  };

  sink_t::sink_t(
    BL::RenderEngine& engine
  , const std::string& view
  , const std::string& layer
  , uint32_t width
  , uint32_t height)
    : details(new details_t(engine, view, layer, width, height))
  {}

  void sink_t::add_tile(
    const Imath::V2i& pos
  , const Imath::V2i& size
  , const Imath::Color3f* splats)
  {
    std::vector<float> pixels(size.x*size.y*4);

    auto i=0;

    for (auto y=size.y-1; y>=0; --y) {
      for (auto x=0; x<size.x; ++x, ++i) {
	const auto index = (y * size.x + x) * 4;
	pixels[index  ] = splats[i].x;
	pixels[index+1] = splats[i].y;
	pixels[index+2] = splats[i].z;
	pixels[index+3] = 1.0f;
      }
    }

    details->m.lock();

    auto result = details->begin(pos, size);

    BL::RenderResult::layers_iterator layer;
    result.layers.begin(layer);

    details->pass(result, *layer, pixels.data());
    details->send(result);

    details->m.unlock();
  }
}
