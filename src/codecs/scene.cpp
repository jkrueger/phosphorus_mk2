#include "scene.hpp"
#include "../scene.hpp"
#include "scene/entities.hpp"
#include "scene/alembic.hpp"
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

    void import(const std::string& path, scene_t& scene) {
      const auto config = YAML::LoadFile(path);
      const auto base   = fs::basepath(path);

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
    }
  }
}
