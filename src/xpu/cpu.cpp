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

  camera_kernel_t            camera_rays;
  stream_mbvh_kernel_t       trace;
  deferred_shading_kernel_t  interactions;
  spt::light_sampler_t       sample_lights;
  spt::integrator_t          integrate;

  details_t(const parsed_options_t& options)
    : trace(&accel)
    , sample_lights(options)
    , integrate(options)
  {}
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
	// create append only memory allocator
	allocator_t allocator(1024*1024*100);

        auto trace_state = stream_mbvh_kernel_t::make_state();

	job::tiles_t::tile_t tile;
	while (frame.tiles->next(tile)) {
	  auto splats = new(allocator) Imath::Color3f[tile.w * tile.h];
          memset(splats, 0, sizeof(Imath::Color3f) * tile.w*tile.h);

	  for (auto i=0; i<spp; ++i) {
	    allocator_scope_t scope(allocator);

            auto state = new(allocator) spt::state_t<>(&scene, frame.sampler);

	    auto rays = new(allocator) ray_t<>();
            auto primary = new(allocator) interaction_t<>();
            auto hits = new(allocator) interaction_t<>();

	    const auto& samples = frame.sampler->next_pixel_samples(i);
	    const auto& camera  = scene.camera;

	    active_t<> active;
	    active.reset(0);

	    details->camera_rays(camera, tile, samples, rays);

            details->trace(trace_state, rays, active);
            details->interactions(allocator, scene, active, rays, primary);
            details->sample_lights(state, active, primary, primary, rays);
            details->trace(trace_state, rays, active);
            details->integrate(state, active, primary, rays);

	    while (active.has_live_paths()) {
	      allocator_scope_t scope(allocator);

              details->trace(trace_state, rays, active);
	      details->interactions(allocator, scene, active, rays, hits);
	      details->sample_lights(state, active, primary, hits, rays);
	      details->trace(trace_state, rays, active);
	      details->integrate(state, active, hits, rays);
            }

	    // TODO: apply fiter

	    // FIXME: temporary code to copy radiance values into output buffer
	    // this should run through a filter kernel instead
	    for (auto j=0; j<tile.w*tile.h; ++j) {
	      const auto r = state->r.at(j);
	      splats[j] += r * (1.0f / (spp*pps));
	    }
	  }

	  frame.film->add_tile(
	    Imath::V2i(tile.x, tile.y)
	  , Imath::V2i(tile.w, tile.h)
	  , splats);

	  allocator.reset();
	}

        delete trace_state;
        
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
