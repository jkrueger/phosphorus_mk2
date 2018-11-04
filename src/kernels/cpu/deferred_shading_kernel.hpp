#pragma once

#include "mesh.hpp"
#include "utils/allocator.hpp"
#include "utils/assert.hpp"

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
    assert(state);

    deferred_t deferred(allocator, scene.num_materials());
    sort_by_material(scene, state, active, deferred);

    assert(deferred.size == scene.num_materials());
    for (auto i=0; i<deferred.size; ++i) {
      const auto material = scene.material(i);
      assert(material->id == i);
      material->evaluate(allocator, scene, state, deferred.material[i]);
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
	assert(state->shading.mesh[index] < scene.num_meshes());
	const auto mesh = scene.mesh(state->shading.mesh[index]);
	assert(mesh);
	assert(mesh->material(state->shading.set[index]) < scene.num_materials());
	const auto material = mesh->material(state->shading.set[index]);
	
	deferred.material[material].add(index);
      }
    }
  }
};
