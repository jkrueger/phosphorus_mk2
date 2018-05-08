#include "codecs/scene.hpp"
#include "options.hpp"
#include "scene.hpp"
#include "state.hpp"
#include "xpu.hpp"

#include <iostream>
#include <vector>

#include <getopt.h>

/* ! available arguments to the renderer */
static option options[] = {
  { "output",     0,           NULL, 'o' },
  { "no-gpu",     0,           NULL, 'c' },
  { "one-thread", no_argument, NULL, '1' }
};

void usage() {
  std::cerr
    << "usage: phosphorus [-o file] [--output file] scene"
    << std::endl;
}

bool parse_args(int argc, char** argv, parsed_options_t& parsed) {
  char ch;

  while ((ch = getopt_long(argc, argv, "o:", options, nullptr)) != -1) {
    switch (ch) {
    case 'o':
      parsed.output = optarg;
      break;
    case '1':
      parsed.single_threaded = true;
      break;
    case 'c':
      parsed.host_only = true;
      break;
    case '?':
    default:
      break;
    }
  }

  const auto remaining = argc - optind;

  if (remaining < 1) {
    std::cerr << "Need a scene file : " << argc << std::endl;
    return false;
  }

  if (remaining != 1) {
    std::cerr << "Unrecognized extra arguments: " << argc << std::endl;
    return false;
  }

  // the last argument passed is the scene file we want to render
  parsed.scene = argv[argc-1];

  return true;
}

template<typename T>
void preprocess(const T& devices, scene_t& scene) {
  scene.preprocess();

  for(auto& device: devices) { 
    device->preprocess(scene);
  }
}

template<typename T>
void start_devices(const T& devices, const scene_t& scene, frame_state_t& state) {
  for(auto& device: devices) { 
    device->start(scene, state);
  }
}

template<typename T>
void join(const T& devices) {
  for(auto& device : devices) {
    device->join();
  }
}

int main(int argc, char** argv) {
  parsed_options_t options;

  if (!parse_args(argc, argv, options)) {
    usage();
    return -1;
  }

  std::cout << "Importing scene: " << options.scene << std::endl;
  scene_t scene;
  codec::scene::import(options.scene, scene);
  
  std::cout << "Discovering devices" << std::endl;
  // start rendering process
  const auto devices = xpu_t::discover(options);

  std::cout << "Preprocessing" << std::endl;
  preprocess(devices, scene);

  std::cout << "Rendering..." << std::endl;
  frame_state_t state(job::tiles_t::make(1920, 1080, 32));
  start_devices(devices, scene, state);
  join(devices);
  
  std::cout << "Done" << std::endl;
  return 0;
}
