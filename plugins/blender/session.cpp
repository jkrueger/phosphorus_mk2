#include "session.hpp"

#include "buffer.hpp"
#include "jobs/tiles.hpp"
#include "options.hpp"
#include "sink.hpp"
#include "xpu.hpp"

#include "import.hpp"

#include <iostream>

#include <unistd.h>

namespace blender {
  std::string session_t::resources = "";
  std::string session_t::path = "";

  struct session_t::details_t {
    BL::Preferences preferences;
    BL::RenderEngine engine;
    BL::BlendData data;
    BL::Depsgraph depsgraph;
    BL::Scene scene;
    BL::SpaceView3D v3d;
    BL::RegionView3D rv3d;

    // specifies the requested channels in the renderer output
    render_buffer_t::descriptor_t buffer_format;

    uint32_t width, height;

    struct renderer_t {
      parsed_options_t options;
      scene_t scene;
      std::vector<xpu_t*> devices;
    } renderer;

    details_t(
      BL::RenderEngine& engine
    , BL::Preferences& userpref
    , BL::BlendData& data)
      : preferences(userpref)
      , engine(engine)
      , data(data)
      , depsgraph(PointerRNA_NULL)
      , scene(PointerRNA_NULL)
      , v3d(PointerRNA_NULL)
      , rv3d(PointerRNA_NULL)
      , width(0)
      , height(0)
    {}

    details_t(
      BL::RenderEngine& engine
    , BL::Preferences& userpref
    , BL::BlendData& data
    , BL::SpaceView3D& v3d
    , BL::RegionView3D& rv3d
    , int width
    , int height)
      : preferences(userpref)
      , engine(engine)
      , data(data)
      , depsgraph(PointerRNA_NULL)
      , scene(PointerRNA_NULL)
      , v3d(v3d)
      , rv3d(rv3d)
      , width(width)
      , height(height)
    {}

    void render(const std::string& view, const std::string& layer) {
      const auto w = render_width();
      const auto h = render_height();

      auto tiles = job::tiles_t::make(w, h, 32, buffer_format);
      auto sink = new sink_t(engine, view, layer, w, h);
      auto sampler = new sampler_t(renderer.options);

      frame_state_t state(sampler, tiles, sink);

      sampler->preprocess(renderer.scene);

      for(auto& device: renderer.devices) { 
        device->start(renderer.scene, state);
      }

      join();

      delete sink;
      delete tiles;
      delete sampler;
    }

    void join() {
      for(auto& device: renderer.devices) {
        device->join();
      }
    }

    void build_scene(BL::RenderSettings& settings) {
      renderer.scene.reset();
      import::import(settings, engine, scene, depsgraph, data, renderer.scene);
      renderer.scene.preprocess();
    }

    void render_options() {
      PointerRNA pscene = RNA_pointer_get(&scene.ptr, "phosphoros");
      
      renderer.options.single_threaded = true; // RNA_int_get(&pscene, "single_threaded");
      renderer.options.samples_per_pixel = RNA_int_get(&pscene, "samples_per_pixel");
      renderer.options.paths_per_sample = RNA_int_get(&pscene, "paths_per_sample");
      renderer.options.path_depth = RNA_int_get(&pscene, "max_path_depth");
    }

    void init_sub_systems(const std::string& path) {
      material_t::boot(renderer.options, path);
    }

    void prepare_devices() {
      for(auto& device: renderer.devices) { 
        delete device;
      }

      renderer.devices.clear();
      renderer.devices = xpu_t::discover(renderer.options);

      for(auto& device: renderer.devices) { 
        device->preprocess(renderer.scene);
      }
    }

    /* determines the requested render passes (lifted from cycles) */
    void determine_render_passes(BL::RenderResult& result) {
      buffer_format.reset();

      BL::RenderResult::layers_iterator first_render_layer;
      result.layers.begin(first_render_layer);
      BL::RenderLayer render_layer = *first_render_layer;

      BL::RenderLayer::passes_iterator pi;
      for (render_layer.passes.begin(pi); pi != render_layer.passes.end(); ++pi) {
        BL::RenderPass pass(*pi);

        if (pass.name() == "Combined") {
          std::cout << "Adding primary channel" << std::endl;
          buffer_format.request(render_buffer_t::PRIMARY, 4);
        }
        else if (pass.name() == "Normal") {
          std::cout << "Adding normal channel" << std::endl;
          buffer_format.request(render_buffer_t::NORMALS, 3);
        }
      }
    }

    bool has_lights() const {
      return renderer.scene.num_lights() > 0;
    }

    uint32_t render_width() const {
      return width ? width : renderer.scene.camera.film.width;
    }

    uint32_t render_height() const {
      return height ? height : renderer.scene.camera.film.height;
    }
  };

  session_t::session_t(
    BL::RenderEngine& engine
  , BL::Preferences& userpref
  , BL::BlendData& data
  , bool preview_osl)
    : details(new details_t(engine, userpref, data))
  {
  }

  session_t::session_t(
    BL::RenderEngine& engine
  , BL::Preferences& userpref
  , BL::BlendData& data
  , BL::SpaceView3D& v3d
  , BL::RegionView3D& rv3d
  , int width
  , int height)
    : details(new details_t(engine, userpref, data, v3d, rv3d, width, height))
  {
  }

  void session_t::reset(BL::BlendData& data, BL::Depsgraph& depsgraph)
  {
    details->depsgraph = depsgraph;
    details->scene = depsgraph.scene_eval();
    
    details->render_options();
    details->init_sub_systems(path);

    if (details->rv3d) {
      auto settings = details->scene.render();
      details->build_scene(settings);
    }
    else {
      auto settings = details->engine.render();
      details->build_scene(settings);
    }

    details->prepare_devices();
  }

  void session_t::render(BL::Depsgraph& depsgraph) {
    if (!details->has_lights()) {
      // TODO: log no lights
      std::cout << "No lights" << std::endl;
      return;
    }

    auto view_layer = depsgraph.view_layer_eval();

    auto result = details->engine.begin_result(0, 0, 1, 1, view_layer.name().c_str(), NULL);
    details->determine_render_passes(result);

    // render all views
    BL::RenderResult::views_iterator vi;
    for (result.views.begin(vi); vi!=result.views.end(); ++vi) {
      details->engine.active_view_set(vi->name().c_str());
      // TODO: sync view settings
      details->render(vi->name(), view_layer.name());
    }

    details->engine.end_result(result, false, false, false);
  }
}
