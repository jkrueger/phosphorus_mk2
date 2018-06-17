#pragma once

#include "mesh.hpp"

struct deferred_shading_kernel_t {
  struct deferred_t {
    active_t<>* material;
    uint32_t    size;

    inline deferred_t(uint32_t size)
      : size(size)
    {
      material = new active_t<>[size];
    }
  };

  inline void operator()(
      const scene_t& scene
    , pipeline_state_t<>* state
    , const active_t<>& active) const
  {
    deferred_t deferred(scene.num_materials());
    sort_by_material(scene, state, active, deferred);

    for (auto i=0; i<deferred.size; ++i) {
      scene.material(i)->evaluate(scene, state, deferred.material[i]);
    }

    // TODO: copy deferred buckets back into the active set?
    // this could be helpful for integration
  }

  void sort_by_material(
    const scene_t& scene
  , pipeline_state_t<>* state
  , const active_t<>& active
  , deferred_t& deferred) const
  {
    // TODO: with more materials a radix sort might be faster
    // than bucket sort

    for (auto i=0; i<active.num; ++i) {
      if (state->is_hit(i)) {
	const auto mesh     = scene.mesh(state->shading.mesh[i]);
	const auto material = scene.material(mesh->material(state->shading.set[i]));

	deferred.material[material->id].add(active.index[i]);
      }
    }
  }
};
