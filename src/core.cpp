#include "scene.hpp"
#include "codecs/scene.hpp"

#include <iostream>
#include <vector>

#include <getopt.h>

/* ! Parsed command line options */
struct parsed_options_t {
  std::string scene;
  std::string output;

  inline parsed_options_t()
    : output("out.exr")
  {}
};

/* ! available arguments to the renderer */
static option options[] = {
  { "output", 0, NULL, 'o' }
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
    case '?':
    default:
      break;
    }
  }

  const auto remaining = argc - optind;

  if (remaining < 1) {
    std::cerr << "No scene file given: " << argc << std::endl;
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

int main(int argc, char** argv) {
  parsed_options_t options;

  if (!parse_args(argc, argv, options)) {
    usage();
    return -1;
  }

  scene_t scene;
  codec::scene::import(options.scene, scene);

  return 0;
}
