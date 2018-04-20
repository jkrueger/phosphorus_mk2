#pragma once

#include "entities/camera.hpp"

#include <yaml-cpp/yaml.h>

/**
 * ! YAML importer code for all common renderer entities
 */
namespace YAML {
  /* ! Camera importer */
  template<>
  struct convert<camera_t> {
    static bool decode(const Node& node, camera_t& camera) {
      if (!node.IsMap()) {
	return false;
      }

      const auto pos = node["position"].as<Imath::V3f>();
      const auto at  = node["at"].as<Imath::V3f>();
      const auto up  = node["up"].as<Imath::V3f>();

      camera.focal_length =
	node["focal-length"].as<float>(35.0f);
      camera.sensor_width =
	node["sensor-width"].as<float>(32.0f);

      return true;
    }
  };
}
