#include "shader.hpp"

namespace blender {
  namespace shader {
    const generic_node_t compiler_t::add_shader(
      "add_node",
      /* inputs */ {{ "Shader", "A" }, { "Shader.001", "B" }, { "Shader_001", "B" }}, 
      /* outputs */ {{ "Shader", "Cout" }});

    const mix_rgb_node_t compiler_t::mix_rgb;

    const generic_node_t compiler_t::mix_shader(
      "mix_closure_node",
      /* inputs */ {{ "Fac", "fac" }, { "Shader", "A" }, { "Shader.001", "B" }, { "Shader_001", "B" }}, 
      /* outputs */ {{ "Shader", "Cout" }});

    const generic_node_t compiler_t::luminance(
      "luminance_node",
      /* inputs */ {{ "Color", "in" }},
      /* outputs */ {{ "Val", "out" }});

    const generic_node_t compiler_t::fresnel(
      "fresnel_dielectric_node",
      /* inputs */ {{ "IOR", "IoR" }, {"Normal", "shadingNormal", true}},
      /* outputs */ {{ "Fac", "out" }});

    const generic_node_t compiler_t::blackbody(
      "blackbody_node",
      /* inputs */ {{ "Temperature", "temperature" }},
      /* outputs */ {{ "Color", "out" }});

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

    const generic_node_t compiler_t::emission_bsdf(
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

    const generic_node_t compiler_t::normal_map(
      "normal_map_node",
      /* inputs */ {{ "Color", "sample" }, { "Strength", "strength" }},
      /* outputs */ {{ "Normal", "Normal" }},
      {"geom:tangent"});

    const generic_node_t compiler_t::material(
      "material_node",
      /* inputs */ {{ "Surface", "Cs" }},
      /* outputs */ {});

    const compiler_t* compiler_t::factory(BL::ShaderNode& node) {
      if (node.is_a(&RNA_ShaderNodeAddShader)) { return &add_shader; }
      if (node.is_a(&RNA_ShaderNodeMixShader)) { return &mix_shader; }
      
      if (node.is_a(&RNA_ShaderNodeMixRGB)) { return &mix_rgb; }
      if (node.is_a(&RNA_ShaderNodeRGBToBW)) { return &luminance; }

      if (node.is_a(&RNA_ShaderNodeFresnel)) { return &fresnel; }
      if (node.is_a(&RNA_ShaderNodeBlackbody)) { return &blackbody; }
      
      if (node.is_a(&RNA_ShaderNodeBsdfDiffuse)) { return &diffuse_bsdf; }
      if (node.is_a(&RNA_ShaderNodeBsdfGlossy)) { return &glossy_bsdf; }
      if (node.is_a(&RNA_ShaderNodeBsdfVelvet)) { return &sheen_bsdf; }
      if (node.is_a(&RNA_ShaderNodeBsdfRefraction)) { return &refraction_bsdf; }
      if (node.is_a(&RNA_ShaderNodeBsdfTransparent)) { return &transparent_bsdf; }
      if (node.is_a(&RNA_ShaderNodeBsdfGlass)) { return &glass_bsdf; }
      if (node.is_a(&RNA_ShaderNodeEmission)) { return &emission_bsdf; }
      

      if (node.is_a(&RNA_ShaderNodeBackground)) { return &background; }

      if (node.is_a(&RNA_ShaderNodeTexImage)) { return &image_texture; }
      if (node.is_a(&RNA_ShaderNodeTexEnvironment)) { return &env_texture; }
      if (node.is_a(&RNA_ShaderNodeNormalMap)) { return &normal_map; }

      if (node.is_a(&RNA_ShaderNodeOutputMaterial)) { return &material; }
      if (node.is_a(&RNA_ShaderNodeOutputWorld)) { return &material; }

      std::cout << "Unsupported shader type: " << node.name() << std::endl;
      return nullptr;
    }
  }
}
