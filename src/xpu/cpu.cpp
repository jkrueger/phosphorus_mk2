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
  std::vector<std::thread> threads;

  accel::mbvh_t accel;

  camera_kernel_t           camera_rays;
  stream_mbvh_kernel_t      trace;
  deferred_shading_kernel_t interactions;
  spt::light_sampler_t      sample_lights;
  spt::integrator_t         integrate;

  details_t(const parsed_options_t& options)
    : trace(&accel)
    , sample_lights(options)
    , integrate(options)
  {}
};

template<typename Accel>
struct tile_renderer_t {
  typedef typename Accel::state_t accel_state_t;
  typedef spt::state_t<> integrator_state_t;

  const cpu_t::details_t* details;

  allocator_t         allocator;
  accel_state_t*      trace_state;
  integrator_state_t* integrator_state;
  active_t<>          active;
  ray_t<>*            rays;
  interaction_t<>*    primary;
  interaction_t<>*    hits;

  inline tile_renderer_t(
    const cpu_t::details_t* details
  , const scene_t& scene
  , sampler_t* sampler)
    : details(details)
    , allocator(1024*1024*100)
    , trace_state(stream_mbvh_kernel_t::make_state())
  {
    integrator_state = new(allocator) spt::state_t<>(&scene, sampler);
    rays = new(allocator) ray_t<>();
    primary = new(allocator) interaction_t<>();
    hits = new(allocator) interaction_t<>();

    active.reset(0);
  }

  inline ~tile_renderer_t() {
    stream_mbvh_kernel_t::destroy_state(trace_state);
  }

  inline void trace_primary_rays(const scene_t& scene) {
    trace_rays(scene, primary);
  }

  inline void trace_and_advance_paths(const scene_t& scene) {
    trace_rays(scene, hits);
  }

  inline void trace_rays(const scene_t& scene, interaction_t<>* out) {
    details->trace(trace_state, rays, active);
    details->interactions(allocator, scene, active, rays, out);
    details->sample_lights(integrator_state, active, primary, out, rays);
    details->trace(trace_state, rays, active);
    details->integrate(integrator_state, active, out, rays);
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

        tile_renderer_t<stream_mbvh_kernel_t> state(details, scene, frame.sampler);

	job::tiles_t::tile_t tile;
	while (frame.tiles->next(tile)) {
	  auto splats = new(state.allocator) Imath::Color3f[tile.w * tile.h];
          memset(splats, 0, sizeof(Imath::Color3f) * tile.w*tile.h);

	  for (auto i=0; i<spp; ++i) {
	    allocator_scope_t scope(state.allocator);

	    const auto& samples = frame.sampler->next_pixel_samples(i);
	    const auto& camera  = scene.camera;

	    details->camera_rays(camera, tile, samples, state.rays);
            state.trace_primary_rays(scene);

	    while (state.active.has_live_paths()) {
	      allocator_scope_t scope(state.allocator);
              state.trace_and_advance_paths(scene);
            }

	    // FIXME: temporary code to copy radiance values into output buffer
	    // this should run through a filter kernel instead
	    for (auto j=0; j<tile.w*tile.h; ++j) {
	      const auto r = state.integrator_state->r.at(j);
	      splats[j] += r * (1.0f / (spp*pps));
	    }
	  }

	  frame.film->add_tile(
	    Imath::V2i(tile.x, tile.y)
	  , Imath::V2i(tile.w, tile.h)
	  , splats);

	  state.allocator.reset();
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
