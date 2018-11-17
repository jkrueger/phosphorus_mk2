#pragma once

#include "../../material.hpp"
#include "utils/color.hpp"

#include <yaml-cpp/yaml.h>

#include <string>

namespace YAML {
  template<>
  struct convert<color_t> {
    static Node encode(const color_t& rhs) {
      Node rgb;
      rgb.push_back(rhs.r);
      rgb.push_back(rhs.g);
      rgb.push_back(rhs.b);

      Node out;
      out["type"]  = "rgb";
      out["value"] = rgb;
      
      return out;
    }

    static bool decode(const Node& node, color_t& rhs) {
      const auto t = node["type"].as<std::string>();
      const auto v = node["value"];

      if(!v.IsSequence() || v.size() != 3) {
	return false;
      }
      
      if (t == "rgb") {
	rhs.r = v[0].as<float>();
	rhs.g = v[1].as<float>();
	rhs.b = v[2].as<float>();
      
	return true;
      }

      return false;
    }
  };

  template<>
  struct convert<material_t*> {
    static bool decode(const Node& node, material_t*& material) {
      if (!node.IsMap()) {
	return false;
      }
      material = new material_t();

      material_t::builder_t::scoped_t builder(material->builder());

      const auto parameters=node["parameters"];
      for (auto i=parameters.begin(); i!=parameters.end(); ++i) {
	const auto name = (*i)["name"].as<std::string>();
	const auto type = (*i)["type"].as<std::string>();
	if (type == "float") {
	  builder->parameter(name, (*i)["value"].as<float>());
	}
	else if (type == "rgb") {
	  builder->parameter(name, i->as<color_t>());
	}
        else if(type == "string") {
          builder->parameter(name, (*i)["value"].as<std::string>());
        }
	else {
	  throw std::runtime_error("Unknown parameter type: " + type);
	}
      }

      const auto shaders = node["shaders"];
      for (auto i=shaders.begin(); i!=shaders.end(); ++i) {
	builder->shader(
	  (*i)["name"].as<std::string>()
	, (*i)["layer"].as<std::string>()
	, (*i)["type"].as<std::string>("surface"));
      }

      const auto edges = node["connect"];
      for (auto edge : edges) {
        const auto from = edge["from"];
        const auto to   = edge["to"];
        builder->connect(
          from["slot"].as<std::string>(),
          to["slot"].as<std::string>(),
          from["layer"].as<std::string>(),
          to["layer"].as<std::string>());
      }

      return true;
    }
  };
}
