#include "sink.hpp"

#include "buffer.hpp"

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
      // the sink gets accessed by multiple threads, so lock it
      lock();

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

    void pass(BL::RenderResult& result, BL::RenderPass& pass, const render_buffer_t::channel_t* channel) {
      std::vector<float> pixels(channel->width() * channel->height() * channel->components);

      for (int y=0; y<channel->height(); ++y) {
        for (int x=0; x<channel->width(); ++x) {
          const auto index = ((channel->height() - (y + 1)) * channel->width() + x) * channel->components;
          channel->get(x, y, pixels.data() + index);
          
          // set alpha to one for now, as the renderer doesn't produce any 
          // alpha output
          if (channel->name == render_buffer_t::PRIMARY) {
            pixels[index + 3] = 1.0f;
          }
        }
      }

      pass.rect(pixels.data());
      engine.update_result(result);
    }

    void send(BL::RenderResult result) {
      engine.end_result(result, 0, 0, true);
      // don't forget to release lock after sending the data
      unlock();
    }

    void lock() {
      m.lock();
    }

    void unlock() {
      m.unlock();
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
  , const render_buffer_t& buffer)
  {
    if (auto result = details->begin(pos, size)) {
      BL::RenderResult::layers_iterator layer;
      result.layers.begin(layer);

      if (auto pass = layer->passes.find_by_name("Combined", details->view.c_str())) {
        details->pass(result, pass, buffer.channel(render_buffer_t::PRIMARY));
      }

      if (auto pass = layer->passes.find_by_name("Normal", details->view.c_str())) {
        details->pass(result, pass, buffer.channel(render_buffer_t::NORMALS));
      }

      details->send(result);
    }
    else {
      details->unlock();
      return;
    }
  }
}
