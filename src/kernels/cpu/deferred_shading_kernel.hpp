#pragma once

#include "light.hpp"
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
  , const active_t<>& active
  , const ray_t<>* rays
  , interaction_t<>* hits) const
  {
    deferred_t by_material(allocator, scene.num_materials());
    build_interactions(scene, active, rays, hits, by_material);

    for (auto i=0; i<by_material.size; ++i) {
      const auto material = scene.material(i);
      material->evaluate(allocator, hits, by_material.material[i]);
    }

    // TODO: copy deferred buckets back into the active set?
    // this could be helpful for integration
  }

  void build_interactions(
    const scene_t& scene
  , const active_t<>& active
  , const ray_t<>* rays
  , interaction_t<>* hits
  , deferred_t& deferred) const
  {
    // TODO: with more materials a radix sort might be faster
    // than bucket sort

    for (auto i=0; i<active.num; ++i) {
      const auto index = active.index[i];

      hits->flags[index] = rays->flags[index];

      const auto p = rays->p.at(index);
      const auto wi = rays->wi.at(index);

      hits->p.from(index, p + wi * rays->d[index]);
      hits->e.from(index, Imath::Color3f(0));

      if (rays->is_hit(index)) {
	const auto mesh = scene.mesh(rays->meshid(index));
	const auto material = rays->matid(index);

        hits->wi.from(index, -wi);

        mesh->shading_parameters(rays, hits, index);

        deferred.material[material].add(index);
      }
      else {
        hits->wi.from(index, wi);
        if (const auto env = scene.environment()) {
          deferred.material[env->matid()].add(index);
        }
      }
    }
  }
};
