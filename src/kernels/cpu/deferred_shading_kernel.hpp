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
      hits->flags[i] = rays->flags[i];

      const auto p = rays->p.at(i);
      const auto wi = rays->wi.at(i);

      hits->p.from(i, p + wi * rays->d[i]);
      hits->e.from(i, Imath::Color3f(0));

      if (rays->is_hit(i)) {
	const auto mesh = scene.mesh(rays->meshid(i));
	const auto material = rays->matid(i);

        hits->wi.from(i, -wi);

        mesh->shading_parameters(rays, hits, i);

        deferred.material[material].add(i);
      }
      else {
        hits->wi.from(i, wi);
        if (const auto env = scene.environment()) {
          deferred.material[env->matid()].add(i);
        }
      }
    }
  }
};
