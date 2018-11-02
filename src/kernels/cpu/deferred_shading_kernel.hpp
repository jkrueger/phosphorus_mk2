#pragma once

#include "mesh.hpp"
#include "utils/allocator.hpp"

struct deferred_shading_kernel_t {
  struct deferred_t {
    active_t<>* material;
    uint32_t    size;

    inline deferred_t(allocator_t& allocator, uint32_t size)
      : size(size)
    {
      material = new(allocator) active_t<>[size];
    }
  };

  inline void operator()(
      allocator_t& allocator
    , const scene_t& scene
    , pipeline_state_t<>* state
    , const active_t<>& active) const
  {
    deferred_t deferred(allocator, scene.num_materials());
    sort_by_material(scene, state, active, deferred);

    for (auto i=0; i<deferred.size; ++i) {
      scene.material(i)->evaluate(allocator, scene, state, deferred.material[i]);
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
      const auto index = active.index[i];
      if (state->is_hit(index)) {
	const auto mesh     = scene.mesh(state->shading.mesh[index]);
	const auto material = scene.material(mesh->material(state->shading.set[index]));

	deferred.material[material->id].add(active.index[index]);
      }
    }
  }
};
