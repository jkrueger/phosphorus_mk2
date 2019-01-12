#pragma once

#include "state.hpp"

#include <string>

struct allocator_t;
struct color_t;
struct parsed_options_t;
struct scene_t;

struct material_t {
  struct details_t;

  details_t* details;

  struct builder_t {
    typedef std::unique_ptr<builder_t> scoped_t;

    virtual ~builder_t()
    {}

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
    , const Imath::Color3f& c) = 0;

    virtual void parameter(
      const std::string& name
    , const std::string& s) = 0;
  };

  uint32_t id;

  material_t();
  ~material_t();

  builder_t* builder();

  void evaluate(
    allocator_t& allocator
  , interaction_t<>* hits
  , const active_t<>& active);

  void evaluate(
    const Imath::V3f& p
  , const Imath::V3f& wi
  , const Imath::V3f& n
  , const Imath::V2f& st
  , shading_result_t& result);

  bool is_emitter() const;

  static void attach();

  static void boot(const parsed_options_t& options, const std::string& path = "");
};
