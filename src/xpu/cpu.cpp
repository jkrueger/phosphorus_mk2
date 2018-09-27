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
  spt::sampler_t             sample_lights;
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
	  auto state  = new(allocator) pipeline_state_t<>();
	  auto splats = new(allocator) color_t[tile.w * tile.h];

	  // acquire a set of samples from the unit square, to generate
	  // rays for this tile
	  const auto& samples = frame.sampler.next_pixel_samples();
	  const auto& camera  = scene.camera;

	  active_t<> active;
	  active.reset(0);

	  details->camera_rays(camera.to_world, tile, samples, state);
	  details->trace(state, active);
	  details->shade(scene, state, active);

	  auto shadow_samples = new(allocator) occlusion_query_state_t<>();

	  details->sample_lights(scene, state, active, shadow_samples);
	  details->trace(shadow_samples, active);
	  details->integrate(state, active, shadow_samples);
	  // compute next path elements
	  // apply filter

	  // FIXME: temporary code to copy radiance values into output buffer
	  // this should run through a filter kernel instead
	  for (auto i=0; i<tile.w*tile.h; ++i) {
	    const auto r = state->shading.r.at(i);
	    splats[i] = color_t(r);
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
  const auto concurrency = 1;
  options.single_threaded ? 1 : std::thread::hardware_concurrency();

  return new cpu_t(concurrency);
}
