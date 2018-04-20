#pragma once

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-register"
#include <ImathMatrix.h>
#include <ImathVec.h>
#pragma clang diagnostic pop

#include <yaml-cpp/yaml.h>

/**
 * ! YAML importer code for all common renderer entities
 */
namespace YAML {
  /* ! 3d vector importer */
  template<>
  struct convert<Imath::V3f> {
    static bool decode(const Node& node, Imath::V3f& v) {
      if (!node.IsSequence() || node.size() != 3) {
	return false;
      }

      v = Imath::V3f(
        node[0].as<float>()
      , node[1].as<float>()
      , node[2].as<float>());

      return true;
    }
  };
  /* ! 4x4 matriximporter */
  template<>
  struct convert<Imath::M44f> {
    static bool decode(const Node& node, Imath::M44f& m) {
      if (!node.IsMap() || node.size() != 3) {
	return false;
      }

      const auto position = node["position"].as<Imath::V3f>();
      const auto rotation = node["rotation"].as<Imath::V3f>();
      const auto scale    = node["scale"].as<Imath::V3f>();

      m.scale(scale);
      m.rotate(rotation);
      m.translate(position);

      return true;
    }
  };
}
