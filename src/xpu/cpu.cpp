#include "cpu.hpp"
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
};

template<typename Accel>
struct tile_renderer_t {
  typedef spt::state_t<> integrator_state_t;

  // rendering kernel functions
  camera_kernel_t           camera_rays;
  Accel                     trace;
  deferred_shading_kernel_t shade;
  spt::light_sampler_t      prepare_occlusion_queries;
  spt::integrator_t         integrate;

  // renderer state
  integrator_state_t* integrator_state;

  // per tile state
  allocator_t      allocator;
  active_t<>       active;
  ray_t<>*         rays;
  interaction_t<>* primary;
  interaction_t<>* hits;

  inline tile_renderer_t(const cpu_t* cpu, const scene_t& scene, frame_state_t& frame)
    : trace(&cpu->details->accel)
    , prepare_occlusion_queries(cpu->details->options)
    , integrate(cpu->details->options)
    , allocator(1024*1024*100)
  {
    integrator_state = new(allocator) spt::state_t<>(&scene, frame.sampler);
  }

  inline void prepare_tile() {
    rays = new(allocator) ray_t<>();
    primary = new(allocator) interaction_t<>();
    hits = new(allocator) interaction_t<>();

    active.reset(0);
  }

  inline void finalize_tile() {
    allocator.reset();
  }

  inline void prepare_sample(
    job::tiles_t::tile_t& tile
  , uint32_t sample
  , const scene_t& scene
  , frame_state_t& frame)
  {
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
    integrate(integrator_state, active, out, rays);
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
  {
    details->accel.reset();
    
    accel::mbvh_t::builder_t::scoped_t builder(details->accel.builder());

    std::vector<triangle_t> triangles;
    scene.triangles(triangles);

    bvh::from(builder, triangles);
  }
}

void cpu_t::start(const scene_t& scene, frame_state_t& frame) {
  for (auto i=0; i<concurrency; ++i) {
    details->threads.push_back(std::thread(
      [&](const scene_t& scene, frame_state_t& frame) {
	// create per thread state in the shading system
	material_t::attach();

	job::tiles_t::tile_t tile;
	while (frame.tiles->next(tile)) {
          tile_renderer_t<stream_mbvh_kernel_t> renderer(this, scene, frame);
          renderer.prepare_tile();

	  auto splats = new(renderer.allocator) Imath::Color3f[tile.w * tile.h];
          memset(splats, 0, sizeof(Imath::Color3f) * tile.w*tile.h);

	  for (auto j=0; j<spp; ++j) {
	    allocator_scope_t scope(renderer.allocator);

            renderer.prepare_sample(tile, j, scene, frame);
            renderer.trace_primary_rays(scene);

	    while (renderer.active.has_live_paths()) {
	      allocator_scope_t scope(renderer.allocator);
              renderer.trace_and_advance_paths(scene);
            }

	    // FIXME: temporary code to copy radiance values into output buffer
	    // this should run through a filter kernel instead
	    for (auto k=0; k<tile.w*tile.h; ++k) {
	      const auto r = renderer.integrator_state->r.at(k);
	      splats[k] += r * (1.0f / (spp*pps));
	    }
	  }

	  frame.film->add_tile(
	    Imath::V2i(tile.x, tile.y)
	  , Imath::V2i(tile.w, tile.h)
	  , splats);

          renderer.finalize_tile();
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
