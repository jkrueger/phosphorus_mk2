#pragma once

#include "bsdf.hpp"
#include "light.hpp"
#include "mesh.hpp"
#include "utils/allocator.hpp"
#include "utils/assert.hpp"

/** A shading kernel that assume that every surface is a
 *  diffuse reflector, without evaluating any materials. This
 *  is mostly used for testing, and debugging scenes 
 */
struct diffuse_shading_kernel_t {
  inline void operator()(
    allocator_t& allocator
  , const scene_t& scene
  , const active_t& active
  , const rays_t& rays
  , interactions_t& hits) const
  {
    for (auto i=0; i<active.num; ++i) {
      auto& hit = hits[i];
      auto& ray = rays[i];

      hit.flags = ray.flags;

      hit.p = ray.p + ray.wi * ray.d;
      hit.e = Imath::Color3f(0.0f);

      if (ray.is_hit()) {
        const auto mesh = scene.mesh(ray.meshid());
        
        hit.wi = -ray.wi;

        mesh->shading_parameters(ray, hit);

        bsdf::lobes::diffuse_t lobe{hit.n};

        hit.bsdf = new(allocator) bsdf_t();
        hit.bsdf->add_lobe(
          bsdf_t::Diffuse
        , Imath::Color3f(1.0f)
        , &lobe);
      }
      /* else {
        /hit.wi = ray.wi;
        if (const auto env = scene.environment()) {
          deferred.material[env->matid()].add(i);
        }
      }*/
    }
  }
};
