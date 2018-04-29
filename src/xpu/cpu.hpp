#pragma once

#include "xpu.hpp"

#include <stdint.h>

struct frame_state_t;
struct parsed_options_t;
struct scene_t;

/* Implements logic for X86 intel CPUs */
struct cpu_t : public xpu_t {
  struct details_t;

  details_t* details;

  // number of worker threads on this cpu
  uint32_t concurrency;
  // TODO reference to work queues, for rays and for shading

  cpu_t(uint32_t concurrency);
  ~cpu_t();

  void preprocess(const scene_t& scene);

  // start the rendering process on all worker threads
  void start(const scene_t& scene, frame_state_t& state);

  // join the host worker threads
  void join();

  static cpu_t* make(const parsed_options_t& options);
};
