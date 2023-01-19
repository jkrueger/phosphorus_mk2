#include "shader.hpp"

namespace blender {
  namespace shader {
    const generic_node_t compiler_t::add_shader(
      "add_node",
      /* inputs */ {{ "Shader", "A" }, { "Shader.001", "B" }, { "Shader_001", "B" }}, 
      /* outputs */ {{ "Shader", "Cout" }});

    const layer_weight_node_t compiler_t::layer_weight;

    const input_node_t compiler_t::const_color(
      "const_color_node",
      "Color",
      {{ "Color", "out", }});

    const light_path_node_t compiler_t::light_path;

    const property_node_t compiler_t::object_info(
      std::vector<property_node_t::property_t>(
        {{ "input/property_node", "Random", "object:random" }}));

    const property_node_t compiler_t::geometry(
      std::vector<property_node_t::property_t>(
        {{ "input/position_node", "Position" },
         { "input/normal_node", "Normal" },
         { "input/incoming_node", "Incoming" },
         { "input/backfacing_node", "Backfacing" }}));

    const property_node_t compiler_t::tex_coordinate(
      /*properties*/ std::vector<property_node_t::property_t>({{"input/uv_node", "UV" }}));

    const math_node_t compiler_t::math;

    const mix_rgb_node_t compiler_t::mix_rgb;

    const rgb_curves_node_t compiler_t::rgb_curves;

    const color_ramp_node_t compiler_t::color_ramp;

    const generic_node_t compiler_t::invert(
      "invert_node",
      /* inputs */ {{ "Fac", "fac" }, { "Color", "in" }}, 
      /* outputs */ {{ "Color", "out" }});

    const generic_node_t compiler_t::transform_point(
      "transform_point_node",
      /* inputs */ {{ "Vector", "in" }, { "Scale", "scale" }, { "Rotation", "rotation" }, { "Location", "translation" }}, 
      /* outputs */ {{ "Vector", "out" }});

    const generic_node_t compiler_t::transform_vector(
      "transform_vector_node",
      /* inputs */ {{ "Vector", "in" }, { "Scale", "scale" }, { "Rotation", "rotation" }}, 
      /* outputs */ {{ "Vector", "out" }});

    const generic_node_t compiler_t::split(
      "split_node",
      /* inputs */ {{ "Image", "in" }}, 
      /* outputs */ {{ "R", "r" }, { "G", "g"}, {"B", "b"}});

    const generic_node_t compiler_t::combine(
      "combine_node",
      /* inputs */ {{ "Color", "in" }, { "Fac", "fac" }, { "R", "r" }, { "G", "g" }, { "B", "b" }}, 
      /* outputs */ {{ "Image", "out" }});

    const generic_node_t compiler_t::hsv(
      "hsv_node",
      /* inputs */ {{ "Hue", "hue" }, { "Saturation", "saturation" }, { "Value", "value" }}, 
      /* outputs */ {{ "Color", "out" }});

    const generic_node_t compiler_t::mix_shader(
      "mix_closure_node",
      /* inputs */ {{ "Fac", "fac" }, { "Shader", "A" }, { "Shader.001", "B" }, { "Shader_001", "B" }}, 
      /* outputs */ {{ "Shader", "Cout" }});

    const generic_node_t compiler_t::luminance(
      "luminance_node",
      /* inputs */ {{ "Color", "in" }},
      /* outputs */ {{ "Val", "out" }});

    const generic_node_t compiler_t::gamma(
      "gamma_node",
      /* inputs */ {{ "Color", "in" }, { "Gamma", "gamma" }},
      /* outputs */ {{ "Color", "out" }});

    const generic_node_t compiler_t::fresnel(
      "fresnel_dielectric_node",
      /* inputs */ {{ "IOR", "IoR" }, {"Normal", "shadingNormal", true}},
      /* outputs */ {{ "Fac", "out" }});

    const generic_node_t compiler_t::blackbody(
      "blackbody_node",
      /* inputs */ {{ "Temperature", "temperature" }},
      /* outputs */ {{ "Color", "out" }});

    const generic_node_t compiler_t::principled_bsdf(
      "principled_bsdf_node",
      /* inputs */ {{ "Base Color", "base" },
                    { "Metallic", "metallic" },
                    { "Specular", "specular_tint" },
                    { "Roughness", "roughness" },
                    { "Sheen", "sheen_weight" },
                    { "Sheen Tint", "sheen_tint" },
                    { "Clearcoat", "clearcoat" },
                    { "Clearcoat Roughness", "clearcoat_roughness" },
                    { "IOR", "ior" },
                    { "Transmission", "transmission" },
                    { "Transmission Roughness", "transmission_roughness" },
                    {"Normal", "shading_normal", true}, 
                    {"Clearcoat Normal", "clearcoat_normal", true}},
      /* outputs */ {{ "BSDF", "out" }});

    const generic_node_t compiler_t::diffuse_bsdf(
      "diffuse_bsdf_node",
      /* inputs */ {{ "Color", "Cs" }, {"Normal", "shadingNormal", true}},
      /* outputs */ {{ "BSDF", "Cout" }});

    const glossy_node_t compiler_t::glossy_bsdf;

    const glass_node_t compiler_t::glass_bsdf;

    const generic_node_t compiler_t::sheen_bsdf(
      "sheen_bsdf_node",
      /* inputs */ {{ "Color", "Cs" }, {"Sigma", "roughness"}, {"Normal", "shadingNormal", true}},
      /* outputs */ {{ "BSDF", "Cout" }});

    const refraction_node_t compiler_t::refraction_bsdf;

    const generic_node_t compiler_t::transparent_bsdf(
      "transparent_bsdf_node",
      /* inputs */ {{ "Color", "Cs" }},
      /* outputs */ {{ "BSDF", "Cout" }});

    const generic_node_t compiler_t::emission(
      "diffuse_emitter_node",
      /* inputs */ {{ "Color", "Cs" }, {"Strength", "power"}},
      /* outputs */ {{ "Emission", "Cout" }});

    const generic_node_t compiler_t::background(
      "background_node",
      /* inputs */ {{ "Color", "Cs" }, {"Strength", "power"}},
      /* outputs */ {{ "Background", "Cout" }});

    const image_texture_node_t compiler_t::image_texture;

    const env_texture_node_t compiler_t::env_texture;

    const generic_node_t compiler_t::random_noise_2d(
      "random_noise_2d_node",
      /* inputs */ {{"Position", "position"}, 
                    {"Scale", "scale"},
                    {"Detail", "detail"}, 
                    {"Roughness", "roughness"}, 
                    {"Distortion", "distortion"}},
      /* outputs */ {{ "Fac", "fac" }, { "Color", "out" }});

    const generic_node_t compiler_t::random_noise_3d(
      "random_noise_3d_node",
      /* inputs */ {{"Position", "position"}, 
                    {"Scale", "scale"},
                    {"Detail", "detail"}, 
                    {"Roughness", "roughness"}, 
                    {"Distortion", "distortion"}},
      /* outputs */ {{ "Fac", "fac" }, { "Color", "out" }});

    const generic_node_t compiler_t::musgrave_noise_3d(
      "musgrave_noise_3d_node",
      /* inputs */ {{"Position", "position"}, 
                    {"Scale", "scale"},
                    {"Octaves", "octaves"}, 
                    {"Lacunarity", "lacunarity"}},
      /* outputs */ {{ "Color", "out" }});

    const generic_node_t compiler_t::normal_map(
      "normal_map_node",
      /* inputs */ {{ "Color", "sample" }, { "Strength", "strength" }},
      /* outputs */ {{ "Normal", "Normal" }},
      {"geom:tangent"});

    const group_node_t compiler_t::group;
    
    const group_input_node_t compiler_t::group_input;
    const group_output_node_t compiler_t::group_output;
    
    const generic_node_t compiler_t::material(
      "material_node",
      /* inputs */ {{ "Surface", "Cs" }},
      /* outputs */ {});

    const compiler_t* compiler_t::factory(BL::ShaderNode& node) {
      if (node.is_a(&RNA_ShaderNodeAddShader)) { return &add_shader; }
      if (node.is_a(&RNA_ShaderNodeMixShader)) { return &mix_shader; }
      
      if (node.is_a(&RNA_ShaderNodeRGB)) { return &const_color; }
      if (node.is_a(&RNA_ShaderNodeTexCoord)) { return &tex_coordinate; }
      if (node.is_a(&RNA_ShaderNodeLightPath)) { return &light_path; }
      if (node.is_a(&RNA_ShaderNodeNewGeometry)) { return &geometry; }
      if (node.is_a(&RNA_ShaderNodeObjectInfo)) { return &object_info; }
      
      if (node.is_a(&RNA_ShaderNodeLayerWeight)) { return &layer_weight; }

      if (node.is_a(&RNA_ShaderNodeMath)) { return &math; }
      if (node.is_a(&RNA_ShaderNodeMixRGB)) { return &mix_rgb; }
      if (node.is_a(&RNA_ShaderNodeRGBCurve)) { return &rgb_curves; }
      if (node.is_a(&RNA_ShaderNodeValToRGB)) { return &color_ramp; }
      if (node.is_a(&RNA_ShaderNodeInvert)) { return &invert; }
      if (node.is_a(&RNA_ShaderNodeSeparateRGB)) { return &split; }
      if (node.is_a(&RNA_ShaderNodeCombineRGB)) { return &combine; }
      if (node.is_a(&RNA_ShaderNodeHueSaturation)) { return &hsv; }

      if (node.is_a(&RNA_ShaderNodeRGBToBW)) { return &luminance; }
      if (node.is_a(&RNA_ShaderNodeGamma)) { return &gamma; }

      if (node.is_a(&RNA_ShaderNodeMapping)) { 
        BL::ShaderNodeMapping mapping_node(node);
        switch(mapping_node.vector_type()) {
          case BL::ShaderNodeMapping::vector_type_POINT:
            return &transform_point;
          case BL::ShaderNodeMapping::vector_type_VECTOR:
            return &transform_vector;
          default:
            std::cout 
              << "Mapping type: " 
              << mapping_node.vector_type() 
              << " not supported" 
              << std::endl;
            break;
        }
      }

      if (node.is_a(&RNA_ShaderNodeFresnel)) { return &fresnel; }
      if (node.is_a(&RNA_ShaderNodeBlackbody)) { return &blackbody; }
      
      if (node.is_a(&RNA_ShaderNodeBsdfPrincipled)) { return &principled_bsdf; }
      if (node.is_a(&RNA_ShaderNodeBsdfDiffuse)) { return &diffuse_bsdf; }
      if (node.is_a(&RNA_ShaderNodeBsdfGlossy)) { return &glossy_bsdf; }
      if (node.is_a(&RNA_ShaderNodeBsdfVelvet)) { return &sheen_bsdf; }
      if (node.is_a(&RNA_ShaderNodeBsdfRefraction)) { return &refraction_bsdf; }
      if (node.is_a(&RNA_ShaderNodeBsdfTransparent)) { return &transparent_bsdf; }
      if (node.is_a(&RNA_ShaderNodeBsdfGlass)) { return &glass_bsdf; }

      if (node.is_a(&RNA_ShaderNodeEmission)) { return &emission; }
      if (node.is_a(&RNA_ShaderNodeBackground)) { return &background; }

      if (node.is_a(&RNA_ShaderNodeTexImage)) { return &image_texture; }
      if (node.is_a(&RNA_ShaderNodeTexEnvironment)) { return &env_texture; }

      if (node.is_a(&RNA_ShaderNodeTexNoise)) {
        BL::ShaderNodeTexNoise noise_node(node);
        switch(noise_node.noise_dimensions()) {
        case BL::ShaderNodeTexNoise::noise_dimensions_2D:
          return &random_noise_2d;
        case BL::ShaderNodeTexNoise::noise_dimensions_3D:
          return &random_noise_3d;
        default:
          std::cout 
            << "Noise dimensions not supported: " << noise_node.noise_dimensions() 
            << std::endl;
          break;
        } 
      }

      if (node.is_a(&RNA_ShaderNodeNormalMap)) { return &normal_map; }      

      if (node.is_a(&RNA_ShaderNodeGroup)) { return &group; }
      if (node.is_a(&RNA_NodeGroupInput)) { return &group_input; }
      if (node.is_a(&RNA_NodeGroupOutput)) { return &group_output; }

      if (node.is_a(&RNA_ShaderNodeOutputMaterial)) { return &material; }
      if (node.is_a(&RNA_ShaderNodeOutputWorld)) { return &material; }

      std::cout << "Unsupported shader type: " << node.name() << std::endl;
      return nullptr;
    }
  }
}
