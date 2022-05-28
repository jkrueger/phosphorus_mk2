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
    struct refraction_node_t;
    struct glass_node_t;

    struct layer_weight_node_t;
    struct mix_rgb_node_t;
    struct rgb_curves_node_t;
    struct color_ramp_node_t;

    struct image_texture_node_t;
    struct env_texture_node_t;

    struct group_node_t;
    struct group_input_node_t;
    struct group_output_node_t;

    /* represents a shader node/parameter name pair, that is used
     * to link up shader nodes */
    struct socket_t {
      std::pair<std::string, std::string> socket;

      inline socket_t(const std::string& node, const std::string& parameter) 
       : socket(node, parameter)
      {}

      inline std::string node() const {
        return socket.first;
      }

      inline std::string name() const {
        return socket.second;
      }
    };

    typedef std::vector<socket_t> sockets_t;

    namespace details {
      inline std::string from_blender_distribution(int bd) {
        switch(bd) {
        case BL::ShaderNodeBsdfGlossy::distribution_SHARP:
          return "sharp";
        case BL::ShaderNodeBsdfGlossy::distribution_GGX:
          return "ggx";
        case BL::ShaderNodeBsdfGlossy::distribution_BECKMANN:
          return "beckmann";
        default:
          std::cout << "Unsupported microfacet distribution. Defaulting to sharp" << std::endl;
          return "sharp";
        }
      }

      inline std::string from_blender_blend_type(int bd) {
        switch(bd) {
        case BL::ShaderNodeMixRGB::blend_type_MIX:
          return "mix";
        case BL::ShaderNodeMixRGB::blend_type_MULTIPLY:
          return "mul";
        case BL::ShaderNodeMixRGB::blend_type_ADD:
          return "add";
        case BL::ShaderNodeMixRGB::blend_type_DARKEN:
          return "darken";
        case BL::ShaderNodeMixRGB::blend_type_LIGHTEN:
          return "lighten";
        default:
          std::cout << "Unsupported blend type. Defaulting to mul" << std::endl;
          return "mul";
        }
      }
    }

    /* Compiles a shader node into a part of a material */
    struct compiler_t {
      /* Add a blender node to a material */
      virtual void compile(BL::ShaderNode& node, material_t::builder_t::scoped_t& builder, BL::Scene& scene) const = 0;

      /* Get a vector of shader node name, and parameter pairs, for a given blender socket.
       * This is so we can potantially link up one blender node sockets with multiple shader node inputs */
      virtual sockets_t input_socket(BL::NodeSocket& socket) const = 0;

      /* Get a shader node name, and parameter pair, that represent a shader node output */
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
      static const generic_node_t const_color;
      static const layer_weight_node_t layer_weight;
      static const mix_rgb_node_t mix_rgb;
      static const rgb_curves_node_t rgb_curves;
      static const color_ramp_node_t color_ramp;
      static const generic_node_t transform_point;
      static const generic_node_t invert;
      static const generic_node_t split;
      static const generic_node_t combine;     
      static const generic_node_t luminance;
      static const generic_node_t gamma;
      static const generic_node_t fresnel;
      static const generic_node_t principled_bsdf;
      static const generic_node_t diffuse_bsdf;
      static const glossy_node_t glossy_bsdf;
      static const glass_node_t glass_bsdf;
      static const generic_node_t sheen_bsdf;
      static const generic_node_t transparent_bsdf;
      static const refraction_node_t refraction_bsdf;
      static const generic_node_t emission;
      static const generic_node_t background;
      static const image_texture_node_t image_texture;
      static const env_texture_node_t env_texture;
      static const generic_node_t random_noise_2d;
      static const generic_node_t random_noise_3d;
      static const generic_node_t musgrave_noise_3d;
      static const generic_node_t normal_map;
      static const generic_node_t blackbody;
      static const group_node_t group;
      static const group_input_node_t group_input;
      static const group_output_node_t group_output;
      static const generic_node_t material;
    };

    /* create a single shader in the material, and passthrough all variables, 
     * with a mapping from blender names to internal names. this is used for shader nodes
     * that have a more or less 1-to-1 mapping to our OSL shaders */
    struct generic_node_t : public compiler_t {
      struct mapping_t {
        std::string from;
        std::string to;
        bool uninitialized;

        inline mapping_t(const std::string& from, const std::string& to) 
         : from(from)
         , to(to)
         , uninitialized(false)
        {}

        inline mapping_t(const std::string& from, const std::string& to, bool uninitialized) 
         : from(from)
         , to(to)
         , uninitialized(uninitialized)
        {}
      };

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
          if (!i->uninitialized) {
            set_default_from(node, i->from, i->to, builder);
          }
        }

        for (const auto& attribute : attributes) {
          builder->add_attribute(attribute);
        }

        builder->shader(shader, node.name(), "surface");
      }

      virtual sockets_t input_socket(BL::NodeSocket& socket) const {
        const auto out = find_socket(inputs, socket);
        return out ? sockets_t({ out.value() }) : sockets_t();
      }

      virtual std::optional<socket_t> output_socket(BL::NodeSocket& socket) const {
        return find_socket(outputs, socket);
      }

      private:
        std::optional<socket_t> find_socket(const std::vector<mapping_t>& sockets, BL::NodeSocket& socket) const {
          const auto guard = std::find_if(sockets.begin(), sockets.end(), [&](const mapping_t& mapping) {
            return mapping.from == socket.identifier();
          });

          if (guard != sockets.end()) {
            return socket_t{ socket.node().name(), guard->to };
          }

          return {};
        }
    };

    struct mix_rgb_node_t : public generic_node_t {
      mix_rgb_node_t()
       : generic_node_t("mix_color_node", {{ "Fac", "fac" }, { "Color1", "A" }, { "Color2", "B" }}, {{ "Color", "Cout" }})
      {}

      void compile(BL::ShaderNode& node, material_t::builder_t::scoped_t& builder, BL::Scene& scene) const {
        BL::ShaderNodeMixRGB mix_node(node.ptr);

        builder->parameter("operation", details::from_blender_blend_type(mix_node.blend_type()));
        generic_node_t::compile(node, builder, scene);
      }
    };

    struct color_ramp_node_t : public generic_node_t {
      color_ramp_node_t()
       : generic_node_t("color_ramp_node",
          /* inputs */ {{ "Fac", "fac" }}, 
          /* outputs */ {{ "Color", "out" }})
      {}

      void compile(BL::ShaderNode& node, material_t::builder_t::scoped_t& builder, BL::Scene& scene) const {
        BL::ShaderNodeValToRGB ramp_node(node);
        BL::ColorRamp color_ramp(ramp_node.color_ramp());

        std::vector<Imath::Color3f> ramp; ramp.resize(256);
        std::vector<float> alpha; alpha.resize(256);

        for (auto i=0; i<256; ++i) {
          float color[4];

          color_ramp.evaluate((float)i / (float)(255), color);
          ramp[i]  = Imath::V3f(color[0], color[1], color[2]);
          alpha[i] = color[3];
        }

        builder->parameter("ramp_color", ramp);
        generic_node_t::compile(node, builder, scene);
      }
    };

    struct rgb_curves_node_t : public generic_node_t {
      rgb_curves_node_t()
       : generic_node_t("rgb_curves_node",
          /* inputs */ {{ "Fac", "fac" }, { "Color", "in" }}, 
          /* outputs */ {{ "Color", "out" }})
      {}

      void compile(BL::ShaderNode& node, material_t::builder_t::scoped_t& builder, BL::Scene& scene) const {
        BL::ShaderNodeValToRGB ramp_node(node);
        BL::ColorRamp color_ramp(ramp_node.color_ramp());

        std::vector<Imath::Color3f> ramp; ramp.resize(256);
        std::vector<float> alpha; alpha.resize(256);

        for (auto i=0; i<256; ++i) {
          float color[4];

          color_ramp.evaluate((float)i / (float)(255), color);
          ramp[i]  = Imath::V3f(color[0], color[1], color[2]);
          alpha[i] = color[3];
        }

        builder->parameter("ramp_color", ramp);
        generic_node_t::compile(node, builder, scene);
      }
    };

    struct layer_weight_node_t : public compiler_t {
      void compile(BL::ShaderNode& node, material_t::builder_t::scoped_t& builder, BL::Scene& scene) const {
        BL::ShaderNodeBsdfGlossy lw_node(node.ptr);

        BL::NodeSocket fresnel = lw_node.outputs[0];
        BL::NodeSocket facing  = lw_node.outputs[1];

        if (fresnel.is_linked()) {
          set_default_from(node, "Blend", "in", build);
          builder->shader("input/artistic_ior_node");
          builder->shader("fresnel_dielectric_node");

          builder->connect("out", "eta", "input/facing_node", "fresnel_dielectric_node");
        }

        if (facing.is_linked()) {
          builder->shader("input/facing_node");

          set_default_from(node, "Blend", "fac", builder);
          builder->shader("math/bias_node");

          builder->connect("out", "fac", "input/facing_node", "math/bias_node");
        }
      }

      sockets_t input_socket(BL::NodeSocket& socket) const 
      { 
        if (socket.name() == "Blend") {
          return{ { "Blend", "fac" } };
        }

        return sockets_t(); 
      }

      std::optional<socket_t> output_socket(BL::NodeSocket& socket) const {
        if (socket.name() == "Fresnel") {
          return socket_t{ "Fresnel", "out" };
        }
        
        return socket_t{ "Facing", "out" };
      }
    };

    struct glossy_node_t : public generic_node_t {
      glossy_node_t()
       : generic_node_t("glossy_bsdf_node", {
             {"Color", "Cs"}, 
             {"Roughness", "roughness"}, 
             {"Normal", "shadingNormal", true}
           }, 
           {{"BSDF", "Cout"}})
      {}

      void compile(BL::ShaderNode& node, material_t::builder_t::scoped_t& builder, BL::Scene& scene) const {
        BL::ShaderNodeBsdfGlossy glossy_node(node.ptr);

        builder->parameter("distribution", details::from_blender_distribution(glossy_node.distribution()));
        generic_node_t::compile(node, builder, scene);
      }
    };

    struct refraction_node_t : public generic_node_t {
      refraction_node_t()
       : generic_node_t("refraction_bsdf_node", {
             {"Color", "Cs"}, 
             {"Roughness", "roughness"}, 
             {"Normal", "shadingNormal", true},
             {"IOR", "IoR"}
           },
           {{"BSDF", "Cout"}})
      {}

      void compile(BL::ShaderNode& node, material_t::builder_t::scoped_t& builder, BL::Scene& scene) const {
        BL::ShaderNodeBsdfRefraction refraction_node(node.ptr);

        builder->parameter("distribution", details::from_blender_distribution(refraction_node.distribution()));
        generic_node_t::compile(node, builder, scene);
      }
    };

    /* compiles a blender glass node into an osl node group */
    struct glass_node_t : public compiler_t {
      void compile(BL::ShaderNode& node, material_t::builder_t::scoped_t& builder, BL::Scene& scene) const {
        BL::ShaderNodeBsdfGlass glass_node(node.ptr);

        const auto distribution = details::from_blender_distribution(glass_node.distribution());

        const std::string reflection = node.name() + ".reflection";
        const std::string refraction = node.name() + ".refraction";
        const std::string fresnel = node.name() + ".fresnel";
        const std::string output = node.name() + ".output";

        // reflection
        set_default_from(node, "Roughness", "roughness", builder);
        builder->parameter("distribution", distribution);
        builder->shader("glossy_bsdf_node", reflection, "surface");

        // refraction
        set_default_from(node, "Roughness", "roughness", builder);
        set_default_from(node, "IOR", "IoR", builder);
        builder->parameter("distribution", distribution);
        builder->shader("refraction_bsdf_node", refraction, "surface");

        // fresnel blend
        set_default_from(node, "IOR", "IoR", builder);
        builder->shader("fresnel_dielectric_node", fresnel, "surface");

        // mix node
        builder->shader("mix_closure_node", output, "surface");

        builder->connect("out", "fac", fresnel, output);
        builder->connect("Cout", "A", refraction, output);
        builder->connect("Cout", "B", reflection, output);
      }
 
      sockets_t input_socket(BL::NodeSocket& socket) const {
        if (socket.name() == "Roughness") {
          return {
            { socket.node().name() + ".reflection" , "roughness"},
            { socket.node().name() + ".refraction" , "roughness"},
          };
        } else if (socket.name() == "Color") {
          return {
            { socket.node().name() + ".reflection" , "Cs"},
            { socket.node().name() + ".refraction" , "Cs"},
          };
        } else if (socket.name() == "IOR") {
          return { { socket.node().name() + ".fresnel", "IoR" } };
        }

        return {};
      }

      std::optional<socket_t> output_socket(BL::NodeSocket& socket) const {
        return socket_t{ socket.node().name() + ".output", "Cout" };
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

      texture_file_node_t(const std::vector<std::string>& attributes)
       : generic_node_t(node_t<T>::value, {}, {{ "Color", "Cout"}}, attributes)\
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

    struct image_texture_node_t : texture_file_node_t<BL::ShaderNodeTexImage> {
      image_texture_node_t() 
        : texture_file_node_t({ "geom:uv" })
      {}
    };

    struct env_texture_node_t : texture_file_node_t<BL::ShaderNodeTexEnvironment> 
    {};

    struct group_node_t : compiler_t {
      virtual void compile(BL::ShaderNode& node, material_t::builder_t::scoped_t& builder, BL::Scene& scene) const {
        BL::Node::inputs_iterator in;
        for (node.inputs.begin(in); in!=node.inputs.end(); ++in) {
          set_default(*in, "in", builder);
          builder->shader(passthrough_shader(*in), in->identifier(), "surface");
        }

        BL::Node::outputs_iterator out;
        for (node.outputs.begin(out); out!=node.outputs.end(); ++out) {
          builder->shader(passthrough_shader(*out), out->identifier(), "surface");
        }
      }

      inline std::string passthrough_shader(BL::NodeSocket& socket) const {
        const auto& type = socket.type();
        switch(type) {
        case BL::NodeSocket::type_VALUE:
          return "passthrough_float_node";
        case BL::NodeSocket::type_INT:
          return "passthrough_int_node";
        case BL::NodeSocket::type_RGBA:
          return "passthrough_color_node";
        case BL::NodeSocket::type_VECTOR:
          return "passthrough_vector_node";
        case BL::NodeSocket::type_STRING:
          return "passthrough_string_node";
        case BL::NodeSocket::type_SHADER:
          return "passthrough_shader_node";
        default:
          std::cout
            << "Don't know how to passthrough parameter of type: "
            << type
            << std::endl;
          break;
        }

        return "";
      }

      virtual sockets_t input_socket(BL::NodeSocket& socket) const {
        return sockets_t( {{ socket.identifier(), "in" }} );
      }

      virtual std::optional<socket_t> output_socket(BL::NodeSocket& socket) const {
        return socket_t{ socket.identifier(), "out" };
      }
    };
 
    struct group_input_node_t : compiler_t {
      virtual void compile(BL::ShaderNode& node, material_t::builder_t::scoped_t& builder, BL::Scene& scene) const
      {}

      virtual sockets_t input_socket(BL::NodeSocket& socket) const {
        return sockets_t();
      }

      virtual std::optional<socket_t> output_socket(BL::NodeSocket& socket) const {
        return socket_t{ socket.identifier(), "out" };
      }
    };

    struct group_output_node_t : compiler_t {
      virtual void compile(BL::ShaderNode& node, material_t::builder_t::scoped_t& builder, BL::Scene& scene) const
      {}

      virtual sockets_t input_socket(BL::NodeSocket& socket) const {
        return sockets_t({{ socket.identifier(), "in" }});
      }

      virtual std::optional<socket_t> output_socket(BL::NodeSocket& socket) const {
        return {};
      }
    };
  }
}
