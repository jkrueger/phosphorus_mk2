#pragma once

#include <string>

struct scene_t;

namespace codec {
  namespace scene {
    /**
     * ! Imports a scene description in YAML format fromn 'path'
     * and writes the results to 'scene'.
     */
    void import(const std::string& path, scene_t& scene);
  }
}
