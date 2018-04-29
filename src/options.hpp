#pragma once

#include <string>

/* Parsed command line options */
struct parsed_options_t {
  std::string scene;
  std::string output;

  // only use one host thread
  bool single_threaded;
  // don't use gpu resources
  bool host_only;

  inline parsed_options_t()
    : output("out.exr")
    , single_threaded(false)
  {}
};
