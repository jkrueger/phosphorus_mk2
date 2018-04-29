#pragma once

#include "xpu.hpp"

struct parsed_options_t;

/* Implements logic for NVIDA GPUs via the CUDA framework */
struct cuda_t : public xpu_t {

  void preprocess(const scene_t& scene);

  static cuda_t* make(const parsed_options_t& options);
};
