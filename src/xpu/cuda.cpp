#include "cuda.hpp"
#include "options.hpp"
#include "scene.hpp"

void cuda_t::preprocess(const scene_t& scene) {
  // build bh for the cpu, build materials, or do
  // whatever else needs to be done
}

cuda_t* cuda_t::make(const parsed_options_t& options) {
  const auto cuda = new cuda_t();

  return cuda;
}
