#include "cpu.hpp"
#include "options.hpp"
#include "scene.hpp"
#include "state.hpp"

#include "kernels/cpu/camera.hpp"

#include <thread>
#include <vector>

struct cpu_t::details_t {
  std::vector<std::thread> threads;

  // camera_kernel_t generate_primary_rays;
};

cpu_t::cpu_t(uint32_t concurrency)
  : details(new details_t())
  , concurrency(concurrency) {
}

cpu_t::~cpu_t() {
  delete details;
}

void cpu_t::preprocess(const scene_t& scene) {
  // build bvh for the cpu, build materials, or do
  // whatever else needs to be done
}

void cpu_t::start(const scene_t& scene, frame_state_t& frame) {
  printf("start: %d\n", concurrency);
  for (auto i=0; i<concurrency; ++i) {
    details->threads.push_back(std::thread([&](const scene_t& scene, frame_state_t& frame) {
      // TODO: if no shading work is to be done...
      job::tiles_t::tile_t tile;
      if (frame.tiles->next(tile)) {
	pipeline_state_t<>* state = new pipeline_state_t<>();

	// acquire a set of samples from the unit square, to generate
	// rays for this tile
	const auto& camera  = scene.camera;
	const auto& samples = frame.sampler.next_pixel_samples();

	generate_camera_rays(camera.to_world, tile, samples, *state);
      //
      //  details->trace_kernel(state);
      //
      //  sort results by material
      //
      //  generate secondary rays
      //  trace shadow rays
      //  shade
      }
    }, std::cref(scene), std::ref(frame)));
  }
}

void cpu_t::join() {
  printf("join\n");
  for (auto& thread : details->threads) {
    thread.join();
  }
}

cpu_t* cpu_t::make(const parsed_options_t& options) {
  const auto concurrency =
    options.single_threaded ? 1 : std::thread::hardware_concurrency();

  return new cpu_t(concurrency);
}
