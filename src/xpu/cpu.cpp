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
};

template<typename Accel>
struct tile_renderer_t {
  typedef spt::state_t<> integrator_state_t;

  static const uint32_t ALLOCATOR_SIZE = 1024*1024*100; // 100MB
  
  uint32_t spp;
  uint32_t pps;

  frame_state_t& frame;

  // rendering kernel functions
  camera::perspective_kernel_t camera_rays;
  Accel                        trace;
  deferred_shading_kernel_t    shade;
  spt::light_sampler_t         prepare_occlusion_queries;
  spt::integrator_t            integrate;

  // renderer state
  integrator_state_t* integrator_state;

  // per tile state
  allocator_t      allocator;
  active_t<>       active;
  ray_t<>*         rays;
  interaction_t<>* primary;
  interaction_t<>* hits;

  // output buffer for the rendered tile
  render_buffer_t buffer;

  // output channels for this tile
  struct {
    render_buffer_t::channel_t* primary;
    render_buffer_t::channel_t* normals;
  } channels;

  inline tile_renderer_t(const cpu_t* cpu, const scene_t& scene, frame_state_t& frame)
    : spp(cpu->spp)
    , pps(cpu->pps)
    , frame(frame)
    , trace(&cpu->details->accel)
    , prepare_occlusion_queries(cpu->details->options)
    , integrate(cpu->details->options)
    , allocator(ALLOCATOR_SIZE)
    , buffer(frame.tiles->format)
  {
    integrator_state = new(allocator) spt::state_t<>(&scene, frame.sampler);

    channels.primary = buffer.channel(render_buffer_t::PRIMARY);
    channels.normals = buffer.channel(render_buffer_t::NORMALS);
  }

  /* allocate dynamic memory used to render a tile */
  inline void prepare_tile(const job::tiles_t::tile_t& tile) {
    rays = new(allocator) ray_t<>();
    primary = new(allocator) interaction_t<>();
    hits = new(allocator) interaction_t<>();

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
    active.reset(0);

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
  inline void trace_rays(const scene_t& scene, interaction_t<>* out) {
    trace(rays, active);
    shade(allocator, scene, active, rays, out);
    prepare_occlusion_queries(integrator_state, active, primary, out, rays);
    trace(rays, active);
    integrate(integrator_state, active, primary, out, rays);
  }

  inline void render_tile(const job::tiles_t::tile_t& tile, const scene_t& scene) {
    allocator_scope_t tile_scope(allocator);
    prepare_tile(tile);

    for (auto j=0; j<spp; ++j) {
      // free per sample state for every sample
      allocator_scope_t sample_scope(allocator);

      prepare_sample(tile, j, scene);
      trace_primary_rays(scene);

      while (active.has_live_paths()) {
        // clear temporary shading data, for every path segment
        allocator_scope_t inner_scope(allocator);
        trace_and_advance_paths(scene);
      }

      // FIXME: temporary code to copy radiance values into output buffer
      // this should run through a filter kernel instead
      for (auto y=0; y<tile.h; ++y) {
        for (auto x=0; x<tile.w; ++x) {
          const auto k = y * tile.w + x;
          const auto r = integrator_state->r.at(k);

          if (channels.primary) {
            // if (std::isnan(r.x) || std::isnan(r.y) || std::isnan(r.z)) {
            //   std::cout << "Path is Nan" << std::endl;
            // }
            // if (std::isinf(r.x) || std::isinf(r.y) || std::isinf(r.z)) {
            //   std::cout << "Path is Nan" << std::endl;
            // }
            // if (r.x > 1000.0f || r.y > 1000.0f || r.z > 1000.0f) {
            //   std::cout << "Path is Firefly" << std::endl;
            // }

            channels.primary->add(x, y, r * (1.0f / (spp * pps)));
          }

          if (channels.normals && primary->is_hit(k)) {
            channels.normals->set(x, y, primary->n.at(k));
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

        tile_renderer_t<stream_mbvh_kernel_t> renderer(this, scene, frame);

      	job::tiles_t::tile_t tile;
      	while (frame.tiles->next(tile)) {
          renderer.render_tile(tile, scene);
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
