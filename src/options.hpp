#pragma once

#include <string>

/* Parsed command line options */
struct parsed_options_t {
  static const uint32_t DEFAULT_SAMPLES_PER_PIXEL = 64;

  std::string scene;
  std::string output;

  // only use one host thread
  bool single_threaded;
  // don't use gpu resources
  bool host_only;
  // number of pixel samples.
  // if set to 0, sampling is adaptive (not supported yet)
  uint32_t samples_per_pixel;

  inline parsed_options_t()
    : output("out.exr")
    , single_threaded(false)
    , samples_per_pixel(DEFAULT_SAMPLES_PER_PIXEL)
  {}
};
