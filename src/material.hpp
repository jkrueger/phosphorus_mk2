#pragma once

#include "state.hpp"

#include <string>

struct color_t;
struct parsed_options_t;
struct scene_t;

struct material_t {
  struct details_t;

  details_t* details;

  struct builder_t {
    typedef std::unique_ptr<builder_t> scoped_t;

    virtual void shader(
      const std::string& name
    , const std::string& layer
    , const std::string& type) = 0;

    virtual void connect(
      const std::string& src_param
    , const std::string& dst_param
    , const std::string& src_layer
    , const std::string& dst_layer) = 0;

    virtual void parameter(
      const std::string& name
    , float f) = 0;

    virtual void parameter(
      const std::string& name
    , const color_t& c) = 0;
  };

  uint32_t id;

  material_t();
  ~material_t();

  builder_t* builder();

  void evaluate(
    const scene_t& scene
  , pipeline_state_t<>* state
  , const active_t<>& active);

  bool is_emitter() const;

  static void attach();

  static void boot(const parsed_options_t& options);
};
