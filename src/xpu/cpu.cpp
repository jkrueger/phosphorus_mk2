#include "cpu.hpp"

#include "buffer.hpp"
#include "material.hpp"
#include "options.hpp"
#include "scene.hpp"
#include "state.hpp"

#include "accel/bvh.hpp"
#include "accel/bvh/binned_sah_builder.hpp"

#include "kernels/cpu/camera.hpp"
#include "kernels/cpu/stream_bvh_kernel.hpp"
// #include "kernels/cpu/linear_bvh_kernel.hpp"
#include "kernels/cpu/deferred_shading_kernel.hpp"
#include "kernels/cpu/diffuse_shading_kernel.hpp"
#include "kernels/cpu/spt.hpp"

#include "utils/allocator.hpp"

#include <random> 
#include <thread>
#include <vector>

struct cpu_t::details_t {
  parsed_options_t options;
  
  std::vector<std::thread> threads;

  accel::mbvh_t accel;

  details_t(const parsed_options_t& options)    
    : options(options)
  {}

  void reset(const scene_t& scene) {
    accel.reset();
    
    accel::mbvh_t::builder_t::scoped_t builder(accel.builder());

    std::vector<triangle_t> triangles;
    scene.triangles(triangles);

    bvh::from(builder, triangles);
  }

  template<typename Renderer>
  inline void render(
    const scene_t& scene
  , frame_state_t& frame) 
  {
    Renderer renderer(options, accel, scene, frame);

    job::tiles_t::tile_t tile;
    while (frame.tiles->next(tile)) {
      renderer.render_tile(tile, scene);
    }
  }
};

template<typename Accel, typename ShadingKernel>
struct tile_renderer_t {
  typedef spt::state_t integrator_state_t;

  static const uint32_t ALLOCATOR_SIZE = 1024*1024*100; // 100MB
  
  uint32_t spp;
  uint32_t pps;

  frame_state_t& frame;

  // rendering kernel functions
  camera::perspective_kernel_t camera_rays;
  Accel                        trace;
  ShadingKernel                shade;
  spt::light_sampler_t         prepare_occlusion_queries;
  spt::integrator_t            integrate;

  // renderer state
  integrator_state_t* integrator_state;

  // per tile state
  allocator_t    allocator;
  rays_t         rays;
  interactions_t primary;
  interactions_t hits;

  // output buffer for the rendered tile
  render_buffer_t buffer;

  // output channels for this tile
  struct {
    render_buffer_t::channel_t* primary;
    render_buffer_t::channel_t* normals;
  } channels;

  inline tile_renderer_t(const parsed_options_t& options, const accel::mbvh_t& accel, const scene_t& scene, frame_state_t& frame)
    : spp(options.samples_per_pixel)
    , pps(options.paths_per_sample)
    , frame(frame)
    , trace(&accel)
    , prepare_occlusion_queries(options)
    , integrate(options)
    , allocator(ALLOCATOR_SIZE)
    , buffer(frame.tiles->format)
  {
    integrator_state = new(allocator) spt::state_t(&scene, frame.sampler);

    channels.primary = buffer.channel(render_buffer_t::PRIMARY);
    channels.normals = buffer.channel(render_buffer_t::NORMALS);
  }

  /* allocate dynamic memory used to render a tile */
  inline void prepare_tile(const job::tiles_t::tile_t& tile) {
    rays.allocate(allocator);
    primary.allocate(allocator);
    hits.allocate(allocator);

    // memset(primary, 0, sizeof(interaction_t<>));

    // allocate memory based on the tiles render buffer format
    // this will allocate memory for channels in the buffer, like 
    // the primary rneder output, and additional information like
    // normals, depth information for a pixel, and so on
    buffer.allocate(allocator, tile.w, tile.h);
  }

  /* setup primary rays, and reset the integrator state */
  inline void prepare_sample(
    const job::tiles_t::tile_t& tile
  , uint32_t sample
  , const scene_t& scene)
  {
    integrator_state->reset();

    // memset(hits, 0, sizeof(interaction_t<>));

    const auto& samples = frame.sampler->next_pixel_samples(sample);
    const auto& camera  = scene.camera;

    camera_rays(camera, tile, samples, rays);
  }

  inline void trace_primary_rays(const scene_t& scene) {
    trace_rays(scene, primary);
  }

  inline void trace_and_advance_paths(const scene_t& scene) {
    trace_rays(scene, hits);
  }

  /** 
   * The basic render pipleine step. 
   * trace rays -> evaluate materials at hit points -> 
   * find occlusion queries -> trace occlusion query rays -> 
   * compute radiance values -> generate new paths vertices 
   *
   */
  inline void trace_rays(const scene_t& scene, interactions_t& out) {
    trace(rays, integrator_state->active);
    shade(allocator, scene, integrator_state->active, rays, out);
    prepare_occlusion_queries(integrator_state, primary, out, rays);
    trace(rays, integrator_state->active);
    integrate(integrator_state, primary, out, rays);
  }

  inline void render_tile(const job::tiles_t::tile_t& tile, const scene_t& scene) {
    allocator_scope_t tile_scope(allocator);
    prepare_tile(tile);

    for (auto j=0; j<spp; ++j) {
      // free per sample state for every sample
      allocator_scope_t sample_scope(allocator);

      prepare_sample(tile, j, scene);
      trace_primary_rays(scene);

      while (integrator_state->has_live_paths()) {
        // clear temporary shading data, for every path segment
        allocator_scope_t inner_scope(allocator);
        trace_and_advance_paths(scene);
      }

      // FIXME: temporary code to copy radiance values into output buffer
      // this should run through a filter kernel instead
      for (auto y=0; y<tile.h; ++y) {
        for (auto x=0; x<tile.w; ++x) {
          const auto k = y * tile.w + x;
          const auto r = integrator_state->r(k);

          if (channels.primary) {
            channels.primary->add(x, y, r * (1.0f / (spp * pps)));
          }

          if (channels.normals && primary[k].is_hit()) {
            channels.normals->set(x, y, primary[k].n);
          }
        }
      }
    }

    frame.film->add_tile(
      Imath::V2i(tile.x, tile.y)
    , Imath::V2i(tile.w, tile.h)
    , buffer);
  }
};

typedef tile_renderer_t<stream_mbvh_kernel_t, deferred_shading_kernel_t> default_tile_renderer_t;
typedef tile_renderer_t<stream_mbvh_kernel_t, diffuse_shading_kernel_t>  diffuse_tile_renderer_t;


cpu_t::cpu_t(const parsed_options_t& options)
  : details(new details_t(options))
  , concurrency(options.single_threaded ? 1 : std::thread::hardware_concurrency())
  , spp(options.samples_per_pixel)
  , pps(options.paths_per_sample)
{}

cpu_t::~cpu_t() {
  delete details;
}

void cpu_t::preprocess(const scene_t& scene) {
  details->reset(scene);
}

void cpu_t::start(const scene_t& scene, frame_state_t& frame) {
  for (auto i=0; i<concurrency; ++i) {
    details->threads.push_back(std::thread(
      [i, this](const scene_t& scene, frame_state_t& frame) {
      	// create per thread state in the shading system
      	material_t::attach();

        if (details->options.render_diffuse_scene) {
          details->render<diffuse_tile_renderer_t>(scene, frame);
        }
        else {
          details->render<default_tile_renderer_t>(scene, frame);
        }
      }, std::cref(scene), std::ref(frame)));
  }
}

void cpu_t::join() {
  for (auto& thread : details->threads) {
    thread.join();
  }
}

cpu_t* cpu_t::make(const parsed_options_t& options) {
  return new cpu_t(options);
}
