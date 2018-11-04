#pragma once

#include "ImathVec.h"

namespace bsdf {
  namespace lobes {
    struct diffuse_t {
      Imath::V3f n;
    };

    struct reflect_t {
      Imath::V3f n;
      float      eta;
    };

    struct refract_t {
      Imath::V3f n;
      float      eta;
    };
  }
};
