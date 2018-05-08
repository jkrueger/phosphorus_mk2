#pragma once

struct stream_mbvh_kernel_t {

  void preprocess(scene_t& scene);

  /* find the closest intersection point for all rays in the
   * current work item in the pipeline */
  void trace(pipeline_state_t<>& state) const;

  inline void operator()(pipeline_state)t<>& state) const {
    trace(state);
  }
};
