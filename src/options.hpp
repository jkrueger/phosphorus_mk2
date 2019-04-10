#pragma once

#include <string>

/* Parsed command line options */
struct parsed_options_t {
  static const uint32_t DEFAULT_SAMPLES_PER_PIXEL = 16;
  static const uint32_t DEFAULT_PATH_DEPTH = 9;
  static const uint32_t DEFAULT_PATHS_PER_SAMPLE = 16;

  std::string scene;
  std::string output;

  // only use one host thread
  bool single_threaded;
  // don't use gpu resources
  bool host_only;
  // render in progressive mode, rather than tiled
  // (i.e. send full frame tiles, representing one sample each, into the pipeline)
  bool progressive;
  // print statistics while rendering
  bool verbose;
  // number of pixel samples.
  // if set to 0, sampling is adaptive (not supported yet)
  uint32_t samples_per_pixel;
  // paths traced per image sample
  uint32_t paths_per_sample;
  // maximum depth of traced paths
  uint32_t path_depth;

  inline parsed_options_t()
    : output("out.exr")
    , single_threaded(false)
    , progressive(false)
    , verbose(false)
    , samples_per_pixel(DEFAULT_SAMPLES_PER_PIXEL)
    , paths_per_sample(DEFAULT_PATHS_PER_SAMPLE)
    , path_depth(DEFAULT_PATH_DEPTH)
  {}
};
