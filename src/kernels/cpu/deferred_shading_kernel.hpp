#pragma once

#include "light.hpp"
#include "mesh.hpp"
#include "utils/allocator.hpp"
#include "utils/assert.hpp"

struct deferred_shading_kernel_t {
  struct deferred_t {
    active_t* material;
    uint32_t  size;

    inline deferred_t(allocator_t& allocator, uint32_t size)
      : size(size)
    {
      material = new(allocator) active_t[size];
    }
  };

  inline void operator()(
    allocator_t& allocator
  , const scene_t& scene
  , const active_t& active
  , const rays_t& rays
  , interactions_t& hits) const
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
  , const active_t& active
  , const rays_t& rays
  , interactions_t& hits
  , deferred_t& deferred) const
  {
    for (auto i=0; i<active.num; ++i) {
      auto& hit = hits[i];
      auto& ray = rays[i];

      hit.flags = ray.flags;

      hit.p = ray.p + ray.wi * ray.d;
      hit.e = Imath::Color3f(0);

      if (ray.is_hit()) {
	      const auto mesh = scene.mesh(ray.meshid());
	      const auto material = ray.matid();

        hit.wi = -ray.wi;

        mesh->shading_parameters(ray, hit);

        deferred.material[material].add(i);
      }
      else {
        hit.wi = ray.wi;
        if (const auto env = scene.environment()) {
          deferred.material[env->matid()].add(i);
        }
      }
    }
  }
};
