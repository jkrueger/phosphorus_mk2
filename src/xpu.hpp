#pragma once

#include <vector>

struct frame_state_t;
struct parsed_options_t;
struct scene_t;

/**
 * Base type for computation resources in the system
 */
struct xpu_t {

  virtual ~xpu_t();

  /**
   * Allow a processor type to precompute data for all instances
   * of this processor type. 
   */
  virtual void preprocess(const scene_t& scene) = 0;

  /**
   * Start the rendering process on this device
   *
   */
  virtual void start(const scene_t& scene, frame_state_t& state) = 0;

  /**
   * oin the rendering threads
   *
   */
  virtual void join() = 0;

  /**
   * Build a list of devices in the system that can be used
   * a processors for rendering tasks
   *
   */
  static std::vector<xpu_t*> discover(const parsed_options_t& options);
};
