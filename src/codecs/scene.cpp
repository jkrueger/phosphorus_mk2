#include "scene.hpp"
#include "../scene.hpp"
#include "../light.hpp"
#include "scene/alembic.hpp"
#include "scene/entities.hpp"
#include "scene/material.hpp"
#include "utils/filesystem.hpp"

#include <yaml-cpp/yaml.h>

#include <functional>
#include <stdexcept>
#include <unordered_map>

typedef std::function<void (const std::string&, scene_t&)> factory_t;

static std::unordered_map<std::string, factory_t> importers = {
  { ".abc", codec::scene::alembic::import }
};

namespace codec {
  namespace scene {
    void import_scene_data(const std::string& path, scene_t& scene) {
      const auto& importer = importers.find(fs::extension(path));
      if (importer != importers.end()) {
    	importer->second(path, scene);
      }
      else {
    	throw std::runtime_error("No importer for: " + path);
      }
    }

    void import_world_data(const YAML::Node& world, scene_t& scene) {
      if (const auto env = world["environment"]) {
        std::cout << "\tAdding environment light" << std::endl;
        const auto material = scene.material(env.as<std::string>());
        scene.add(light_t::make_infinite(material));
      }
    }

    void import(const std::string& path, scene_t& scene) {
      const auto config = YAML::LoadFile(path);
      const auto base   = fs::basepath(path);

      std::cout << "Importing materials" << std::endl;

      // import materials
      const auto materials = config["materials"];
      for (auto i=materials.begin(); i!=materials.end(); ++i) {
	const auto name = i->first.as<std::string>();
	std::cout << "Material: " << name << "(" << scene.num_materials() << ")" << std::endl;
	scene.add(name, i->second.as<material_t*>());
      }

      std::cout << "Importing scene data" << std::endl;

      // import scene data from files
      const auto scene_data = config["data"];
      for (auto i=scene_data.begin(); i!=scene_data.end(); ++i) {
      	const auto path = (*i)["path"].as<std::string>();
      	import_scene_data(base + "/" + path, scene);
      }

      // import manual camera settings from the configuration
      // that may override imported any camera definitions
      // imported from external scene files, like alembic
      if (const auto camera = config["camera"]) {
      	scene.camera = camera.as<camera_t>();
      }

      // import world settings
      if (const auto world = config["world"]) {
        std::cout << "Loading world data" << std::endl;
        import_world_data(world, scene);
      }
    }
  }
}
