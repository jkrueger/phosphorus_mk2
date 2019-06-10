#pragma once

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-register"
#include <ImathVec.h>
#include <OpenImageIO/ustring.h>
#pragma clang diagnostic pop

#include "math/trigonometry.hpp"

namespace bsdf {
  static const uint32_t DIFFUSE = 1;
  static const uint32_t GLOSSY = 2;
  static const uint32_t SPECULAR = 4;
  static const uint32_t REFLECT = 8;
  static const uint32_t TRANSMIT = 16;

  namespace lobes {
    struct diffuse_t {
      static const uint32_t flags = REFLECT | DIFFUSE;
      
      Imath::V3f n;

      inline void precompute() {
      }
    };

    struct oren_nayar_t {
      static const uint32_t flags = REFLECT | DIFFUSE;

      Imath::V3f n;
      float alpha;
      float a, b;

      inline void precompute() {
        const auto s = trig::radians(alpha);
        const auto s2 = s * s;
        a = 1.0f - (s2 / (2.0f * (s2 + 0.33f)));
        b = 0.45f * s2 / (s2 + 0.09f);
      }
    };

    struct reflect_t {
      static const uint32_t flags = REFLECT | SPECULAR;

      Imath::V3f n;
      float      eta;

      inline void precompute() {
      }
    };

    struct refract_t {
      static const uint32_t flags = TRANSMIT | SPECULAR;

      Imath::V3f n;
      float      eta;

      inline void precompute() {
      }
    };

    struct microfacet_t {
      static const uint32_t flags = REFLECT | GLOSSY;

      static const OIIO_NAMESPACE::ustring GGX;

      // distribution name
      OIIO_NAMESPACE::ustring distribution;

      Imath::V3f n;       // shading normal
      Imath::V3f u;       // unused parameter, appearing in the OSL docs
      float      xalpha;  // horizontal roughness
      float      yalpha;  // vertical roughness
      float      eta;     // index of refraction
      int        refract;  //is this a refracting bsdf (like frosted glas)

      inline bool is_ggx() const {
        return distribution == GGX;
      }

      inline void precompute() {
        xalpha = roughness_to_alpha(xalpha);
        yalpha = roughness_to_alpha(yalpha);
      }

      static inline float roughness_to_alpha(float roughness) {
        roughness = std::max(roughness, (float) 1e-3);
        float x = std::log(roughness);
        return 1.62142f
          + 0.819955f * x
          + 0.1734f * x * x
          + 0.0171201f * x * x * x
          + 0.000640711f * x * x * x * x;
      }
    };

    struct sheen_t {
      static const uint32_t flags = REFLECT | GLOSSY;

      Imath::V3f n; // shading normal
      float      r; // roughness term

      inline void precompute() {
      }
    };
  }
};
