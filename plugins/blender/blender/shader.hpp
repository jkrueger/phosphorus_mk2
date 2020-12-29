#pragma once

// from main source directory
#include "material.hpp"

// from plugin directory
#include "texture.hpp"
#include "util.hpp"

#include <MEM_guardedalloc.h>
#include <RNA_access.h>
#include <RNA_blender_cpp.h>
#include <RNA_types.h>

#include <optional>
#include <stdexcept>

namespace blender {
  /**
   * This name space contains infastructure to compile blender shaders nodes
   * into an OSL shader DAG. There is a general process for doing this, and
   * some special cases where blender nodes don't map 1-to-1 to our OSL shaders
   *
   */
  namespace shader {

    struct generic_node_t;
    struct glossy_node_t;
    struct glass_node_t;

    template<typename T>
    struct texture_file_node_t;

    /* represents a shader node/parameter name pair, that is used
     * to link up shader nodes */
    struct socket_t {
      std::pair<std::string, std::string> socket;

      inline std::string node() const {
        return socket.first;
      }

      inline std::string name() const {
        return socket.second;
      }
    };

    /* Compiles a shader node into a part of a material */
    struct compiler_t {
      /* Add a blender node to a material */
      virtual void compile(BL::ShaderNode& node, material_t::builder_t::scoped_t& builder, BL::Scene& scene) const = 0;

      /* Get a shader node name, and parameter pair, to link up two blender node sockets */
      virtual std::optional<socket_t> input_socket(BL::NodeSocket& socket) const = 0;
      virtual std::optional<socket_t> output_socket(BL::NodeSocket& socket) const = 0;

      /* Set a parameter from the default vlaue of a blender node socket. this will be the
       * default value of parameters that don't get linked to other shader nodes */
      void set_default(BL::NodeSocket& socket, const std::string& to, material_t::builder_t::scoped_t& builder) const {
        const auto& type = socket.type();
        switch(type) {
        case BL::NodeSocket::type_VALUE:
          builder->parameter(to, RNA_float_get(&socket.ptr, "default_value"));
          break;
        case BL::NodeSocket::type_INT:
          builder->parameter(to, RNA_int_get(&socket.ptr, "default_value"));
          break;
        case BL::NodeSocket::type_RGBA:
          {
            Imath::Color3f c;
            RNA_float_get_array(&socket.ptr, "default_value", &c.x);
            builder->parameter(to, c);
            break;
          }
        case BL::NodeSocket::type_VECTOR:
          {
            Imath::V3f v;
            RNA_float_get_array(&socket.ptr, "default_value", &v.x);
            builder->parameter(to, v);
            break;
          }
        case BL::NodeSocket::type_STRING:
          {
            char buf[1024];
            char *str = RNA_string_get_alloc(
              &socket.ptr, "default_value", buf, sizeof(buf));
            builder->parameter(to, std::string(str));
            break;
          }
        case BL::NodeSocket::type_SHADER:
          // Ignore shader type sockets, as these only get values through links to other 
          // shader nodes
          break;
        default:
          std::cout
            << "Don't know how to set parameter " << to << " of type: "
            << type
            << std::endl;
          break;
        }
      }

      void set_default_from(BL::ShaderNode& node, const std::string& from, const std::string& to, material_t::builder_t::scoped_t& builder) const {
        if (auto input = node.inputs[from]) {
          set_default(input, to, builder);
        }
        else {
          std::cout << "Can't find property: " << from << ", in shader " << node.name() << std::endl;
        }
      }

      /* create a compiler based on a blender shader node */
      static const compiler_t* factory(BL::ShaderNode& node);

      static const generic_node_t add_shader;
      static const generic_node_t mix_shader;
      static const generic_node_t mix_rgb;
      static const generic_node_t luminance;
      static const generic_node_t fresnel;
      static const generic_node_t diffuse_bsdf;
      static const glossy_node_t glossy_bsdf;
      static const generic_node_t sheen_bsdf;
      static const generic_node_t transparent_bsdf;
      static const generic_node_t refraction_bsdf;
      static const generic_node_t emission_bsdf;
      static const generic_node_t background;
      static const texture_file_node_t<BL::ShaderNodeTexImage> image_texture;
      static const texture_file_node_t<BL::ShaderNodeTexEnvironment> env_texture;
      static const generic_node_t normal_map;
      static const generic_node_t blackbody;
      static const generic_node_t material;
    };

    /* create a single shader in the material, and passthrough all variables, 
     * with a mapping from blender names to internal names. this is used for shader nodes
     * that have a more or less 1-to-1 mapping to our OSL shaders */
    struct generic_node_t : public compiler_t {
      typedef std::pair<std::string, std::string> mapping_t;

      std::string shader;
      std::vector<mapping_t> inputs;
      std::vector<mapping_t> outputs;
      std::vector<std::string> attributes;

      generic_node_t()
      {}

      generic_node_t(
          const std::string& shader 
        , const std::vector<mapping_t>& inputs
        , const std::vector<mapping_t>& outputs
        , const std::vector<std::string>& attributes = {})
      : shader(shader)
      , inputs(inputs)
      , outputs(outputs)
      , attributes(attributes)
      {}

      virtual void compile(BL::ShaderNode& node, material_t::builder_t::scoped_t& builder, BL::Scene& scene) const {
        for (auto i=inputs.begin(); i!=inputs.end(); ++i) {
          set_default_from(node, i->first, i->second, builder);
        }

        for (const auto& attribute : attributes) {
          builder->add_attribute(attribute);
        }

        builder->shader(shader, node.name(), "surface");
      }

      virtual std::optional<socket_t> input_socket(BL::NodeSocket& socket) const {
        return find_socket(inputs, socket);
      }

      virtual std::optional<socket_t> output_socket(BL::NodeSocket& socket) const {
        return find_socket(outputs, socket);
      }

      private:
        std::optional<socket_t> find_socket(const std::vector<mapping_t>& sockets, BL::NodeSocket& socket) const {
          const auto guard = std::find_if(sockets.begin(), sockets.end(), [&](const mapping_t& mapping) {
            return mapping.first == socket.identifier();
          });

          if (guard != sockets.end()) {
            return socket_t{{ socket.node().name(), guard->second }};
          }

          return {};
        }
    };

    struct glossy_node_t : public generic_node_t {
      glossy_node_t()
       : generic_node_t("glossy_bsdf_node", {{"Color", "Cs"}, {"Roughness", "roughness"}}, {{"BSDF", "Cout"}})
      {}

      void compile(BL::ShaderNode& node, material_t::builder_t::scoped_t& builder, BL::Scene& scene) const {
        BL::ShaderNodeBsdfGlossy glossy_node(node.ptr);

        switch(glossy_node.distribution()) {
        case BL::ShaderNodeBsdfGlossy::distribution_SHARP:
          builder->parameter("distribution", "sharp");
          break;
        case BL::ShaderNodeBsdfGlossy::distribution_GGX:
          builder->parameter("distribution", "ggx");
          break;
        case BL::ShaderNodeBsdfGlossy::distribution_BECKMANN:
          builder->parameter("distribution", "beckmann");
          break;
        default:
          std::cout << "Unsupported microfacet distribution. Defaulting to sharp" << std::endl;
          builder->parameter("distribution", "sharp");
        }

        generic_node_t::compile(node, builder, scene);
      }
    };

    /* compiles a blender glass node into a osl node group */
    struct glass_node_t : public compiler_t {
      void compile(BL::ShaderNode& node, material_t::builder_t::scoped_t& builder, BL::Scene& scene) const {
      }

      std::optional<socket_t> input_socket(BL::NodeSocket& socket) const {
        return {};
      }

      std::optional<socket_t> output_socket(BL::NodeSocket& socket) const {
        return {};
      }
    };

    /* compiles a texture that's backed by a file */
    template<typename T>
    struct texture_file_node_t : public generic_node_t {
      
      template<typename X>
      struct node_t {};

      template<>
      struct node_t<BL::ShaderNodeTexImage> {
        static constexpr const char* value = "texture_node";
      };

      template<>
      struct node_t<BL::ShaderNodeTexEnvironment> {
        static constexpr const char* value = "environment_node";
      };

      texture_file_node_t()
       : generic_node_t(node_t<T>::value, {}, {{ "Color", "Cout"}})
      {}

      void compile(BL::ShaderNode& node, material_t::builder_t::scoped_t& builder, BL::Scene& scene) const {
        T tex(node.ptr);
        BL::Image image(tex.image());
        BL::ImageUser image_user(tex.image_user());

        if (image) {
          const auto frame = const_cast<BL::Scene&>(scene).frame_current();
          const auto path = texture::image_file_path(image_user, image, frame);

          PointerRNA colorspace_ptr = image.colorspace_settings().ptr;
          const auto colorspace = util::get_enum_identifier(colorspace_ptr, "name");

          std::string texture_path = "";

          // we add a texture to the renderer from either a pixel buffer 
          // (i.e. a texture comnig out of blender), or a texture file. in both cases
          // a color space transformation will be applied to the texture, if requested
          // by blender
          if (image.packed_file()) {
            const auto type = texture::to_oiio_buffer_type(image);
            const auto pixels = texture::load_pixels(image);

            texture_path = texture::make(
              path,
              pixels,
              image.size()[0], image.size()[1], image.channels(),
              type,
              colorspace);
          }
          else {
            texture_path = texture::make(path, colorspace);
          }

          // TODO: this would throw, and crash blender, if the texture file could 
          // not be generated. handle this case by inserting some placeholder
          // texture
          builder->parameter("filename", texture_path);
        }
        else {
          std::cout << "Can't find image!" << std::endl;
        }

        generic_node_t::compile(node, builder, scene);
      }
    };
  }
}
