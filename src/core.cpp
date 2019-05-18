#include "codecs/scene.hpp"
#include "film/file.hpp"
#include "material.hpp"
#include "options.hpp"
#include "scene.hpp"
#include "state.hpp"
#include "xpu.hpp"

#include <iostream>
#include <vector>

#include <getopt.h>
#include <sys/time.h>

/* available arguments to the renderer */
static option options[] = {
  { "output",     0,                 NULL, 'o' },
  { "no-gpu",     no_argument,       NULL, 'c' },
  { "one-thread", no_argument,       NULL, '1' },
  { "spp",        required_argument, NULL, 's' },
  { "paths",      required_argument, NULL, 'p' },
  { "depth",      required_argument, NULL, 'd' },
  { "verbose",    no_argument,       NULL, 'v' }
};

void usage() {
  std::cerr
    << "usage: phosphorus <options> scene"
    << std::endl
    << "-o <path>    Output path for the renderer" << std::endl
    << "-s <samples> Anti Aliasing samples per pixel" << std::endl
    << "-p <paths>   Maximum number of paths traces per sample" << std::endl
    << "-d <depth>   Maximum depth of a single path" << std::endl
    << "-v           Print statistics while rendering" << std::endl;
}

bool parse_args(int argc, char** argv, parsed_options_t& parsed) {
  char ch;

  while ((ch = getopt_long(argc, argv, "c1o:p:s:d:", options, nullptr)) != -1) {
    switch (ch) {
    case 'o':
      parsed.output = optarg;
      break;
    case '1':
      std::cout << "Single threaded mode" << std::endl;
      parsed.single_threaded = true;
      break;
    case 'c':
      std::cout << "CPU only mode" << std::endl;
      parsed.host_only = true;
      break;
    case 's':
      std::cout << "Samples per pixel: " << std::atoi(optarg) << std::endl;
      parsed.samples_per_pixel = std::atoi(optarg);
      break;
    case 'p':
      std::cout << "Paths per sample: " << std::atoi(optarg) << std::endl;
      parsed.paths_per_sample = std::atoi(optarg);
      break;
    case 'd':
      std::cout << "Path depth: " << std::atoi(optarg) << std::endl;
      parsed.path_depth = std::atoi(optarg);
      break;
    case 'v':
      parsed.verbose = true;
    case '?':
    default:
      usage();
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
void preprocess(const T& devices, scene_t& scene, frame_state_t& state) {
  scene.preprocess();

  for(auto& device: devices) { 
    device->preprocess(scene);
  }

  state.sampler->preprocess(scene);
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

// std::thread start_stats_printer() {
//   return std::thread([]() {
//     while (rendering) {
      
//     }
//   });
// }

int main(int argc, char** argv) {
  parsed_options_t options;

  if (!parse_args(argc, argv, options)) {
    usage();
    return -1;
  }

  material_t::boot(options);

  std::cout << "Importing scene: " << options.scene << std::endl;
  scene_t scene;
  codec::scene::import(options.scene, scene);

  std::cout << "Discovering devices" << std::endl;
  const auto devices = xpu_t::discover(options);

  film::file_t* sink = new film::file_t(scene.camera.film, options.output);
  sampler_t* sampler = new sampler_t(options);

  frame_state_t state(
    sampler
  , job::tiles_t::make(
      scene.camera.film.width
    , scene.camera.film.height
    , 32)
  , sink);

  std::cout << "Preprocessing" << std::endl;
  preprocess(devices, scene, state);

  std::cout << "Rendering..." << std::endl;

  timeval start;
  gettimeofday(&start, 0);

  // std::thread stats;

  // if (options.verbose) {
  //   stats = start_stats_printer();
  // }

  start_devices(devices, scene, state);
  join(devices);

  timeval end;
  gettimeofday(&end, 0);

  std::cout
    << "Rendering time: "
    << ((end.tv_sec - start.tv_sec) +
        ((end.tv_usec - start.tv_usec) / 1000000.0))
    << std::endl;

  sink->finalize();

  delete sink;
  delete sampler;

  std::cout << "Done" << std::endl;
  
  return 0;
}
