#include "cpu.hpp"
#include "options.hpp"
#include "scene.hpp"
#include "state.hpp"

#include "kernels/cpu/camera.hpp"
#include "kernels/cpu/stream_bvh_kernel.hpp"
#include "accel/bvh.hpp"
#include "accel/bvh/binned_sah_builder.hpp"

#include <thread>
#include <vector>

struct cpu_t::details_t {
  std::vector<std::thread> threads;

  accel::mbvh_t accel;

  camera_kernel_t      camera_rays;
  stream_mbvh_kernel_t trace;

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
	// TODO: if no shading work is to be done...
	pipeline_state_t<>* state = new pipeline_state_t<>();
	
	job::tiles_t::tile_t tile;
	if (frame.tiles->next(tile)) {
	  // acquire a set of samples from the unit square, to generate
	  // rays for this tile
	  const auto& samples = frame.sampler.next_pixel_samples();
	  const auto& camera  = scene.camera;

	  active_t<> active;

	  details->camera_rays(camera.to_world, tile, samples, *state);
	  details->trace(*state, active);
	  //
	  //  sort results by material
	  //
	  //  generate secondary rays
	  //  trace shadow rays
	  //  shade
	  //  compute next path elements
	}
	delete state;
	
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
