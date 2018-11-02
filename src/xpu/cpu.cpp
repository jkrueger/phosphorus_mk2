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
  deferred_shading_kernel_t  shade;
  spt::light_sampler_t       sample_lights;
  spt::integrator_t          integrate;

  details_t()
    : trace(&accel)
  {}
};

cpu_t::cpu_t(uint32_t concurrency)
  : details(new details_t())
  , concurrency(concurrency) {
}

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
  // TODO: precompute CDFs for scene lights, etc.
}

void cpu_t::start(const scene_t& scene, frame_state_t& frame) {
  for (auto i=0; i<concurrency; ++i) {
    details->threads.push_back(std::thread(
      [&](const scene_t& scene, frame_state_t& frame) {
	// create per thread state in the shading system
	material_t::attach();
	// create append only memory allocator
	allocator_t allocator(1024*1024*100);
	
	job::tiles_t::tile_t tile;
	while (frame.tiles->next(tile)) {
	  auto splats = new(allocator) color_t[tile.w * tile.h];

	  for (auto i=0; i<frame.sampler->spp; ++i) {
	    allocator_scope_t scope(allocator);

	    auto state  = new(allocator) pipeline_state_t<>();

	    // acquire a set of samples from the unit square, to generate
	    // rays for this tile
	    const auto& samples = frame.sampler->next_pixel_samples(i);
	    const auto& camera  = scene.camera;

	    active_t<> active;
	    active.reset(0);

	    details->camera_rays(camera, tile, samples, state);

	    do {
	      // automatically free up allocator memory in each
	      // loop iteration
	      allocator_scope_t scope(allocator);

	      details->trace(state, active);

	      // FIME: find the proper place for this
	      for (auto j=0; j<active.num; ++j) {
		const auto index = active.index[j];
		const auto p     = state->rays.p.at(index);
		const auto wi    = state->rays.wi.at(index);
		
		state->rays.p.from(index, p + wi * state->rays.d[index]);
	      }

	      details->shade(allocator, scene, state, active);

	      auto shadow_samples = new(allocator) occlusion_query_state_t<>();

	      details->sample_lights(frame.sampler, state, active, shadow_samples);
	      details->trace(shadow_samples, active);
	      details->integrate(frame.sampler, scene, state, active, shadow_samples);
	      
	    } while (active.has_live_paths());

	      // apply fiter

	    // FIXME: temporary code to copy radiance values into output buffer
	    // this should run through a filter kernel instead
	    for (auto j=0; j<tile.w*tile.h; ++j) {
	      const auto r = state->result.r.at(j);
	      splats[j] += color_t(r) * (1.0f / frame.sampler->spp);
	    }
	  }

	  frame.film->add_tile(
	    Imath::V2i(tile.x, tile.y)
	  , Imath::V2i(tile.w, tile.h)
	  , splats);

	  allocator.reset();
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
  const auto concurrency =
    options.single_threaded ? 1 : std::thread::hardware_concurrency();

  return new cpu_t(concurrency);
}
