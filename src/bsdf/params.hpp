#pragma once

#include <OpenEXR/ImathVec.h>
#include <OpenImageIO/ustring.h>

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
    };

    struct oren_nayar_t {
      static const uint32_t flags = REFLECT | DIFFUSE;

      struct in_t {
        Imath::V3f n;
        float      alpha;
      };

      Imath::V3f n;
      float      a, b;

      inline oren_nayar_t(const in_t* in)
        : n(in->n)
      {
        const auto s = trig::radians(in->alpha);
        const auto s2 = s * s;
        a = 1.0f - (s2 / (2.0f * (s2 + 0.33f)));
        b = 0.45f * s2 / (s2 + 0.09f);
      }
    };

    struct reflect_t {
      static const uint32_t flags = REFLECT | SPECULAR;

      Imath::V3f n;
      float      eta;
    };

    struct refract_t {
      static const uint32_t flags = TRANSMIT | SPECULAR;

      Imath::V3f n;
      float      eta;
    };

    struct microfacet_t {
      static const uint32_t flags = REFLECT | GLOSSY;

      OIIO_NAMESPACE::ustring distribution;
      Imath::V3f n;
      Imath::V3f u;
      float      xalpha;
      float      yalpha;
      float      eta;
      int        refract;
    };
  }
};
