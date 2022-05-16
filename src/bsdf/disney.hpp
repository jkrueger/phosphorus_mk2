#pragma once

#include "params.hpp"
#include "lambert.hpp"
#include "microfacet.hpp"

/* This implements lobes for the Disney principled shader.
 * Based on: https://media.disneyanimation.com/uploads/production/publication_asset/48/asset/s2012_pbs_disney_brdf_notes_v3.pdf */
namespace disney {

  /* Diffuse term in the disney shader. Adds a fresnel term */
  namespace diffuse {
    Imath::Color3f f(
      const bsdf::lobes::diffuse_t& params
    , const Imath::V3f& wi
    , const Imath::V3f& wo)
    {
      const auto fi = fresnel::schlick_weight(params.n.dot(wi));
      const auto fo = fresnel::schlick_weight(params.n.dot(wo));

      return Imath::Color3f(M_1_PI * (1.0f - fi / 2.0f) * (1.0f - fo / 2.0f));
    }
  }

  /* Retro reflection according to Disney paper */
  namespace retro {
    Imath::Color3f f(
      const bsdf::lobes::disney_retro_t& params
    , const Imath::V3f& wi
    , const Imath::V3f& wo)
    {
      const auto wh = (wi + wo).normalize();
      const auto cos_thetad = wi.dot(wh);

      const auto fi = fresnel::schlick_weight(params.n.dot(wi));
      const auto fo = fresnel::schlick_weight(params.n.dot(wo));
      const auto rr = 2.0f * params.roughness * cos_thetad * cos_thetad;

      return Imath::Color3f(M_1_PI * rr * (fi + fo + fi * fo * (rr - 1.0f)));
    }
  }

  /* Disney sheen lobe implementation */
  namespace sheen {
    Imath::Color3f f(
      const bsdf::lobes::diffuse_t& params
    , const Imath::V3f& wi
    , const Imath::V3f& wo)
    {
      const auto wh = (wi + wo).normalize();
      const auto cos_thetad = wi.dot(wh);

      return Imath::Color3f(fresnel::schlick_weight(cos_thetad));
    }
  }

  /* Disney modifications to GGX microfacet distribution, and fresnel terms */
  namespace microfacet {
    struct disney_fresnel_t {
      float metallic;
      Imath::Color3f cspec0;

      inline Imath::Color3f operator()(float cosi, float eta) const {
        return (1.0f - metallic) * Imath::Color3f(fresnel::dielectric(cosi, eta)) +
               metallic * fresnel::schlick(cspec0, cosi);
      }
    };

    struct disney_ggx_t : public ::microfacet::ggx_t 
    {};
  }
}
