#pragma once

#include "entities/camera.hpp"
#include "light.hpp"
#include "material.hpp"
#include "mesh.hpp"
#include "scene.hpp"

#include <MEM_guardedalloc.h>
#include <RNA_access.h>
#include <RNA_blender_cpp.h>
#include <RNA_types.h>

#include <mikktspace.h>

#include <OpenImageIO/imagebufalgo.h>

#include <filesystem>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

extern "C" {
  void BKE_image_user_frame_calc(void *iuser, int cfra);
  void BKE_image_user_file_path(void *iuser, void *ima, char *path);
}

namespace blender {
  bool is_mesh(BL::Object& object) {
    auto data = object.data();
    return data && data.is_a(&RNA_Mesh);
  }

  bool is_material(BL::ID& id) {
    return id && id.is_a(&RNA_Material);
  }

  namespace mikk {
  
    struct user_data_t {
      mesh_t* mesh;

      user_data_t(mesh_t* mesh) 
       : mesh(mesh)
       {}
    };

    int get_num_faces(const SMikkTSpaceContext* context) {
      const user_data_t* data = (const user_data_t*) context->m_pUserData;
      return data->mesh->num_faces;
    }

    int get_num_verts_of_face(const SMikkTSpaceContext*, int) {
      return 3;
    }

    void get_position(
      const SMikkTSpaceContext* context
    , float p[3]
    , const int face
    , const int vert)
    {
      const user_data_t* data = (const user_data_t*) context->m_pUserData;
      const auto mesh = data->mesh;
      const auto index = mesh->faces[face * 3 + vert];
      const auto& vertex = mesh->vertices[index];

      p[0] = vertex.x;
      p[1] = vertex.y;
      p[2] = vertex.z;
    }

    void get_normal(
      const SMikkTSpaceContext* context
    , float n[3]
    , const int face
    , const int vert) {
      
      const user_data_t* data = (const user_data_t*) context->m_pUserData;
      const auto mesh = data->mesh;
      const auto index = mesh->faces[face * 3 + vert];
      const auto& normal = mesh->vertices[index];

      n[0] = normal.x;
      n[1] = normal.y;
      n[2] = normal.z;
    }

    void get_uv(
      const SMikkTSpaceContext* context
    , float out[2]
    , const int face
    , const int vert) {
      
      const user_data_t* data = (const user_data_t*) context->m_pUserData;
      const auto mesh = data->mesh;
      const auto index = mesh->faces[face * 3 + vert];
      const auto& uv = mesh->uvs[index];

      out[0] = uv.x;
      out[1] = uv.y;
    }

    void set_tangent(
      const SMikkTSpaceContext* context
    , const float t[2]
    , const float sign
    , const int face 
    , const int vert) {
      
      const user_data_t* data = (const user_data_t*) context->m_pUserData;
      const auto mesh = data->mesh;
      const auto index = mesh->faces[face * 3 + vert];
      
      mesh->tangents[index] = Imath::V3f(t[0], t[1], t[2]);
    }

    void generate_tangents(mesh_t* mesh) {
	    user_data_t userdata(mesh);

      SMikkTSpaceInterface sm_interface;
      memset(&sm_interface, 0, sizeof(sm_interface));
      
      sm_interface.m_getNumFaces = get_num_faces;
      sm_interface.m_getNumVerticesOfFace = get_num_verts_of_face;
      sm_interface.m_getPosition = get_position;
      sm_interface.m_getTexCoord = get_uv;
      sm_interface.m_getNormal = get_normal;
      sm_interface.m_setTSpaceBasic = set_tangent;

      SMikkTSpaceContext context;
      memset(&context, 0, sizeof(context));
      context.m_pUserData = &userdata;
      context.m_pInterface = &sm_interface;
      
      genTangSpaceDefault(&context);
    }
  }

  namespace import {
    mesh_t* mesh(BL::Depsgraph& graph, BL::BlendData& data, BL::Object& object, const scene_t& scene) {
      BL::Mesh blender_mesh = BL::Mesh(object.data());
      if (blender_mesh.use_auto_smooth()) {
        BL::Depsgraph depsgraph(PointerRNA_NULL);
        blender_mesh = object.to_mesh(false, depsgraph);
      }

      std::cout << "Importing mesh: " << blender_mesh.name() << std::endl;

      if (blender_mesh.use_auto_smooth()) {
        blender_mesh.split_faces(false);
      }

      blender_mesh.calc_loop_triangles();

      const auto num_vertices = blender_mesh.vertices.length();
      const auto num_faces = blender_mesh.loop_triangles.length();
      const auto use_loop_normals = blender_mesh.use_auto_smooth();

      if (num_faces == 0) {
        return nullptr;
      }

      auto bm = object.matrix_world();

      Imath::M44f transform;
      memcpy(&transform, &bm, sizeof(float)*16);

      Imath::M44f normal_transform = transform;
      normal_transform.invert();
      normal_transform.transpose();

      auto mesh = new mesh_t();
      mesh_t::builder_t::scoped_t builder(mesh->builder());

      if (use_loop_normals) {
        builder->set_uvs_per_vertex_per_face();
      }

      uint32_t num_normals = 0;

      BL::Mesh::vertices_iterator v;
      for (blender_mesh.vertices.begin(v); v!=blender_mesh.vertices.end(); ++v) {
        const auto& a = v->co();
        builder->add_vertex(Imath::V3f(a[0], a[1], a[2]) * transform);

        // if (!use_loop_normals) {
          const auto& n = v->normal();
          builder->add_normal((Imath::V3f(n[0], n[1], n[2]) * normal_transform).normalize());

          num_normals++;
        // }
      }

      std::unordered_map<std::string, std::vector<uint32_t>> sets;

      uint32_t num_indices = 0;

      BL::Mesh::loop_triangles_iterator f;
      blender_mesh.loop_triangles.begin(f);
      for (auto id=0; f!=blender_mesh.loop_triangles.end(); ++f, ++id) {
        auto p = blender_mesh.polygons[f->polygon_index()];
        const auto indices = f->vertices();
        const auto smooth = p.use_smooth() || use_loop_normals;

        auto material = blender_mesh.materials[p.material_index()];

        sets[material.name()].push_back(id);

        if (use_loop_normals) {
          const auto ns = f->split_normals();
          for (auto i=0; i<3; ++i) {
            auto n = (Imath::V3f(ns[i*3], ns[i*3+1], ns[i*3+2]) *
              normal_transform).normalize();

            builder->set_normal(indices[i], n);
            // num_normals++;
          }
        }

        builder->add_face(indices[0], indices[1], indices[2], smooth);
        num_indices +=3;
      }

      if (use_loop_normals) {
        if (num_normals != num_indices) {
          std::cout 
            << "Expecting to have a normal per triangle index: " 
            << num_normals
            << " != "
            << num_indices
            << std::endl;
        }
      }
      else {
        if (num_normals != num_vertices) {
          std::cout << "Expecting to have a normal per vertex" << std::endl;
        }
      }

      bool generate_tangents = false;

      for (auto& set : sets) {
        const auto material = scene.material(set.first);

        if (material->has_attribute("geom:tangent")) {
          generate_tangents = true;
        }

        builder->add_face_set(material, set.second);
      }

      uint32_t num_uvs = 0;

      if (blender_mesh.uv_layers.length() != 0) {
        BL::Mesh::uv_layers_iterator l;
        blender_mesh.uv_layers.begin(l);
        std::cout << "Importing UV map coordinates: " << l->data.length() << std::endl;
        //  TODO: allow multiple UV map layers
        // for (l!=blender_mesh.uv_layers.end(); ++l) {
          BL::Mesh::loop_triangles_iterator f;
          blender_mesh.loop_triangles.begin(f);
          for (;f!=blender_mesh.loop_triangles.end(); ++f) {
            const auto& indices = f->loops();
            for (auto i=0; i<3; ++i) {
              const auto& uv = l->data[indices[i]].uv();
              builder->add_uv(Imath::V2f(uv[0], 1.0f - uv[1]));
              ++num_uvs;
            }
          }
          // }
      }

      if (use_loop_normals) {
        if (num_uvs != num_indices) {
          std::cout << "Expecting to have a uv coordinate per triangle index" << std::endl;
        }
      }
      else {
        if (blender_mesh.uv_layers.length() != 0 && num_uvs != num_vertices) {
          std::cout 
          << "Expecting to have a uv coordinate per vertex: " 
          << num_uvs << " != " << num_vertices 
          << std::endl;
        }
      }

      if (generate_tangents) {
        std::cout << "Generate tangents" << std::endl;
        mesh->allocate_tangents();
        mikk::generate_tangents(mesh);
      }
/*
      if (object.data().ptr.data != blender_mesh.ptr.data) {
        object.to_mesh_clear();
      }
*/
      return mesh;
    }

    inline std::string get_enum_identifier(PointerRNA &ptr, const char *name) {
      PropertyRNA *prop = RNA_struct_find_property(&ptr, name);
      const char *identifier = "";
      int value = RNA_property_enum_get(&ptr, prop);

      RNA_property_enum_identifier(NULL, &ptr, prop, value, &identifier);

      return std::string(identifier);
    }

    std::string image_file_path(
      BL::ImageUser& user
    , BL::Image& image
    , int frame)
    {
      if (!user) {
        std::cout << "No image user" << std::endl;
      }

      if (image.packed_file()) {
        return image.name() + "@" + std::to_string(frame);
      }

      char filepath[1024];
      // BKE_image_user_frame_calc(user.ptr.data, frame);
      BKE_image_user_file_path(user.ptr.data, image.ptr.data, filepath);

      return std::string(filepath);
    }

    /* adds a texture from a file to the material system. this will create a temporary file
     * on disk, that is optimized for lookup, and colorspace converted. returns the file system path
     * to the texture */
    std::string make_texture(const std::string& path, const std::string& colorspace) {
      const auto output = fs::temp_directory_path() / fs::path(colorspace + "_" + path).filename();

      OIIO::ImageBuf image(path);
      OIIO::ImageSpec config;
      config.attribute("maketx:incolorspace", colorspace);
      config.attribute("maketx:outcolorspace", "scene_linear");
      config.attribute("maketx:filtername", "lanczos3");
      config.attribute("maketx:colorconfig", session_t::resources + "/2.90/datafiles/colormanagement/config.ocio");

      std::stringstream ss;

      if (!OIIO::ImageBufAlgo::make_texture(OIIO::ImageBufAlgo::MakeTxTexture, image, output.string(), config, &ss)) {
        throw std::runtime_error("Failed to make texture: " + ss.str());
      }

      return output.string();
    }

    /* adds a texture from a buffer to the material system. this will create a temporary file
     * on disk, that is optimized for lookup, and colorspace converted. returns the file system path
     * of the texture */
    std::string make_texture(const std::string& path, void* pixels, const std::string& colorspace) {
      throw std::runtime_error("Adding texture from buffer not implemented yet");
    }

    typedef std::tuple<
      std::string
      , std::function<
          void (material_t::builder_t::scoped_t&, BL::NodeSocket&, const std::string&)
        >
    > parameter_mapping_t;

    typedef std::unordered_map<
      std::string
    , parameter_mapping_t
    > parameter_mappings_t;

    typedef std::tuple<
      // name of the shader
      std::string
      // extra dependencies required by the shader, such as tangent vectors
      // for normal maps, etc. that are computed on demand
    , std::vector<std::string>
      // mapping for input parameters
    , parameter_mappings_t
      // mapping for output parameters
    , parameter_mappings_t
    > node_descriptor_t;

    /* for certain parameter we ignore the blender default value, since there is none
     * initialization is expected to happen in the shader, and generally this will be a
     * parameter that gets it's value from some other node */
    typename parameter_mappings_t::value_type uninitialized(
      const std::string& a
    , const std::string& b)
    {
      return { a, { b, [](material_t::builder_t::scoped_t&, BL::NodeSocket&, const std::string&) {
        // NOOP
        return;
      } } };
    }

    typename parameter_mappings_t::value_type passthrough(
      const std::string& a
    , const std::string& b)
    {
      return { a, { b, [](material_t::builder_t::scoped_t& builder, BL::NodeSocket& i, const std::string& mapped) {
        const auto& type = i.type();
        switch(type) {
        case BL::NodeSocket::type_VALUE:
          builder->parameter(mapped, RNA_float_get(&i.ptr, "default_value"));
          break;
        case BL::NodeSocket::type_INT:
          builder->parameter(mapped, RNA_int_get(&i.ptr, "default_value"));
          break;
        case BL::NodeSocket::type_RGBA:
          {
            Imath::Color3f c;
            RNA_float_get_array(&i.ptr, "default_value", &c.x);
            builder->parameter(mapped, c);
            break;
          }
        case BL::NodeSocket::type_VECTOR:
          {
            Imath::V3f v;
            RNA_float_get_array(&i.ptr, "default_value", &v.x);
            builder->parameter(mapped, v);
            break;
          }
        case BL::NodeSocket::type_STRING:
          {
            char buf[1024];
            char *str = RNA_string_get_alloc(
              &i.ptr, "default_value", buf, sizeof(buf));
            builder->parameter(mapped, std::string(str));
            break;
          }
        default:
          std::cout
            << "Don't know how to set parameter " << mapped << " of type: "
            << type
            << std::endl;
          break;
        }
      } } };
    }

    node_descriptor_t make_shader_node(BL::ShaderNode& node, BL::Scene& scene)
    {
      if (node.is_a(&RNA_ShaderNodeAddShader)) {
        return {
          "add_node",
          {},
          {
            passthrough("Shader", "A"),
            passthrough("Shader_001", "B"),
          },
          {
            passthrough("Shader", "Cout")
          }
        };
      }
      else if (node.is_a(&RNA_ShaderNodeMixShader)) {
        return {
          "mix_node",
          {},
          {
            passthrough("Fac", "fac"),
            passthrough("Shader", "A"),
            passthrough("Shader_001", "B"),
          },
          {
            passthrough("Shader", "Cout")
          }
        };
      }
      else if (node.is_a(&RNA_ShaderNodeMixRGB)) {
        return {
          "color_op_node",
          {},
          {
            passthrough("Fac", "fac"),
            passthrough("Color1", "A"),
            passthrough("Color2", "B"),
          },
          {
            passthrough("Color", "Cout")
          }
        };
      }
      else if (node.is_a(&RNA_ShaderNodeRGBToBW)) {
        return {
          "color_to_value",
          {},
          {
            passthrough("Color", "Cs"),
          },
          {
            passthrough("Val", "value")
          }
        };
      }
      else if (node.is_a(&RNA_ShaderNodeFresnel)) {
        return {
          "fresnel_dielectric_node",
          {},
          {
            passthrough("IOR", "IoR"),
          },
          {
            passthrough("Fac", "Cout")
          }
        };
      }
      else if (node.is_a(&RNA_ShaderNodeBsdfDiffuse)) {
        return {
          "diffuse_bsdf_node",
          {},
          {
            passthrough("Color", "Cs"),
            uninitialized("Normal", "shadingNormal"),
          },
          {
            passthrough("BSDF", "Cout")
          }
        };
      }
      else if (node.is_a(&RNA_ShaderNodeBsdfGlossy)) {
        return {
          "glossy_bsdf_node",
          {},
          {
            { "distribution",
              { "distribution", [=](material_t::builder_t::scoped_t& builder, BL::NodeSocket&, const std::string& parameter) {
                  BL::ShaderNodeBsdfGlossy glossy_node(node.ptr);

                  switch(glossy_node.distribution()) {
                  case BL::ShaderNodeBsdfGlossy::distribution_SHARP:
                    builder->parameter(parameter, "sharp");
                    break;
                  case BL::ShaderNodeBsdfGlossy::distribution_GGX:
                    builder->parameter(parameter, "ggx");
                    break;
                  default:
                    std::cout
                      << "Unsupported microfacet distribution"
                      << std::endl;
                  }
                }
              }
            },
            passthrough("Color", "Cs"),
            uninitialized("Normal", "shadingNormal"),
            passthrough("Roughness", "roughness"),
          },
          {
            passthrough("BSDF", "Cout")
          }
        };
      }
      else if(node.is_a(&RNA_ShaderNodeBsdfVelvet)) {
        return {
          "sheen_bsdf_node",
          {},
          {
            passthrough("Color", "Cs"),
            passthrough("Sigma", "roughness"),
            uninitialized("Normal", "shadingNormal"),
          },
          {
            passthrough("BSDF", "Cout")
          }
        };
      }
      else if(node.is_a(&RNA_ShaderNodeBackground)) {
        return {
          "background_node",
          {},
          {
            passthrough("Color", "Cs"),
            passthrough("Strength", "power"),
          },
          {
            passthrough("Background", "Cout"),
          }
        };
      }
      else if(node.is_a(&RNA_ShaderNodeBsdfRefraction)) {
      }
      else if(node.is_a(&RNA_ShaderNodeBsdfTransparent)) {
      }
      else if(node.is_a(&RNA_ShaderNodeEmission)) {
        return {
          "diffuse_emitter_node",
          {},
          {
            passthrough("Color", "Cs"),
            passthrough("Strength", "power"),
          },
          {
            passthrough("Emission", "Cout")
          }
        };
      }
      else if(node.is_a(&RNA_ShaderNodeTexImage)) {
        return {
          "texture_node",
          {},
          {
            { "filename",
                { "filename",
                    [=](material_t::builder_t::scoped_t& builder
                      , BL::NodeSocket&
                      , const std::string& parameter)
                      {
                        BL::ShaderNodeTexImage tex(node.ptr);
                        BL::Image image(tex.image());
                        BL::ImageUser image_user(tex.image_user());

                        if (image) {
                          const auto frame = const_cast<BL::Scene&>(scene).frame_current();
                          const auto path = image_file_path(image_user, image, frame);

                          PointerRNA colorspace_ptr = image.colorspace_settings().ptr;
                          const auto colorspace = get_enum_identifier(colorspace_ptr, "name");

                          std::string texture_path = "";

                          // we add a texture to the renderer from either a pixel buffer 
                          // (i.e. a texture comnig out of blender), or a texture file. in both cases
                          // a color space transformation will be applied to the texture, if requested
                          // by blender
                          if (image.packed_file()) {
                            texture_path = make_texture(path, image.ptr.data, colorspace);
                          }
                          else {
                            texture_path = make_texture(path, colorspace);
                          }

                          // TODO: this would throw, and crash blender, if the texture file could 
                          // not be generated. handle this case by inserting some placeholder
                          // texture
                          builder->parameter(parameter, texture_path);
                        }
                        else {
                          std::cout << "Can't find image!" << std::endl;
                        }
                      }
                }
            },
            { "extension",
                { "repeat",
                    [=](material_t::builder_t::scoped_t& builder
                      , BL::NodeSocket&
                      , const std::string&)
                      {
                        // TODO: set s, and t wrap
                      }
                }
            },
          },
          {
            passthrough("Color", "Cout")
          }
        };
      }
      else if(node.is_a(&RNA_ShaderNodeTexEnvironment)) {
        return {
          "environment_node",
          {},
          {
            { "filename",
                { "filename",
                    [=](material_t::builder_t::scoped_t& builder
                      , BL::NodeSocket&
                      , const std::string& parameter)
                      {
                        BL::ShaderNodeTexEnvironment env(node.ptr);

                        BL::Image image(env.image());
                        BL::ImageUser image_user(env.image_user());

                        const auto frame = const_cast<BL::Scene&>(scene).frame_current();
                        const auto path = image_file_path(image_user, image, frame);

                        builder->parameter(parameter, path);
                      }
                }
            },
            { "extension",
                { "repeat",
                    [=](material_t::builder_t::scoped_t& builder
                      , BL::NodeSocket&
                      , const std::string&)
                      {
                        // TODO: set s, and t wrap
                      }
                }
            },
          },
          {
            passthrough("Color", "Cout")
          }
        };
      }
      else if (node.is_a(&RNA_ShaderNodeNormalMap)) {
        return {
          "normal_map_node",
          {"geom:tangent"},
          {
            passthrough("Color", "sample"),
            passthrough("Strength", "strength")
          },
          {
            passthrough("Normal", "Normal"),
          },
        };
      }
      else if (node.is_a(&RNA_ShaderNodeOutputMaterial) ||
               node.is_a(&RNA_ShaderNodeOutputWorld)) {
        return {
          "material_node",
          {},
          {
            passthrough("Surface", "Cs")
          },
          {
          }
        };
      }
      else {
        std::cout << "Unsupported node: " << node.name() << std::endl;
      }

      return {
        "<unknown>", {}, {}, {}
      };
    }

    void set_parameters(
      const node_descriptor_t& desc
    , material_t::builder_t::scoped_t& builder
    , BL::ShaderNode& node)
    {
      const auto mappings = std::get<2>(desc);

      BL::Node::inputs_iterator i;
      for (node.inputs.begin(i); i!=node.inputs.end(); ++i) {
        auto guard = mappings.find(i->identifier());
        if (guard != mappings.end()) {
          std::get<1>(guard->second)(builder, *i, std::get<0>(guard->second));
        }
        else {
          // TODO: log skipped parameter
        }
      }

      if(node.is_a(&RNA_ShaderNodeTexEnvironment) ||
         node.is_a(&RNA_ShaderNodeTexImage)) {
        auto guard = mappings.find("filename");
        if (guard != mappings.end()) {
          BL::NodeSocket dummy(PointerRNA_NULL);
          std::get<1>(guard->second)(builder, dummy, std::get<0>(guard->second));
        }
      }
      
      if(node.is_a(&RNA_ShaderNodeBsdfGlossy)) {
        auto guard = mappings.find("distribution");
        if (guard != mappings.end()) {
          BL::NodeSocket dummy(PointerRNA_NULL);
          std::get<1>(guard->second)(builder, dummy, std::get<0>(guard->second));
        }
      }
    }

    void shader_tree(
      BL::ShaderNodeTree& tree
    , BL::Depsgraph& graph
    , BL::Scene& scene
    , material_t::builder_t::scoped_t& builder)
    {
      std::unordered_map<std::string, node_descriptor_t> descriptors;

      std::unordered_map<void*, std::list<PointerRNA>> in;
      std::unordered_map<void*, std::list<PointerRNA>> out;

      // we have to do a topological sort on the nodes, because OSL expects
      // them to get created in the order the links go through the node tree

      BL::NodeTree::links_iterator link;
      for (tree.links.begin(link); link!=tree.links.end(); ++link) {
        if (!link->is_valid()) {
          continue;
        }

        auto from = link->from_socket().node();
        auto to = link->to_socket().node();

        in[to.ptr.data].push_back(from.ptr);
        out[from.ptr.data].push_back(to.ptr);
      }

      std::list<PointerRNA> roots;
      std::list<PointerRNA> sorted;

      BL::ShaderNodeTree::nodes_iterator node;
      for (tree.nodes.begin(node); node!=tree.nodes.end(); ++node) {
        BL::ShaderNode shader_node(*node);
        if (in.find(shader_node.ptr.data) == in.end()) {
          roots.push_back(shader_node.ptr);
        }
      }

      while (!roots.empty()) {
        auto node = roots.front();
        roots.pop_front();

        auto& links = out[node.data];
        for (auto i=links.begin(); i!=links.end(); ++i) {
          in[i->data].remove_if([=](const PointerRNA& x) {
            return x.data == node.data;
          });
          if (in[i->data].empty()) {
            roots.push_back(*i);
          }
        }

        sorted.push_back(node);
      }

      for (auto i=sorted.begin(); i!=sorted.end(); ++i) {
        BL::ShaderNode node(*i);
        if (node.is_a(&RNA_ShaderNodeOutputMaterial) ||
            node.is_a(&RNA_ShaderNodeOutputWorld)) {
          continue;
        }
        else if (node.is_a(&RNA_ShaderNodeGroup)) {
          // TODO: log not supported
        }
        else if (node.is_a(&RNA_NodeGroupInput)) {
          // TODO: log not supported
        }
        else if (node.is_a(&RNA_NodeGroupOutput)) {
          // TODO: log not supported
        }
        else {
          const auto desc = make_shader_node(node, scene);
          if (std::get<0>(desc) != "<unknown>") {
            set_parameters(desc, builder, node);
            builder->shader(std::get<0>(desc), node.name(), "surface");
            for (const auto& attribute : std::get<1>(desc)) {
              builder->add_attribute(attribute);
            }
            descriptors[node.name()] = desc;
          }
        }
      }

      if (auto output_node = tree.get_output_node(2)) {
        const auto desc = make_shader_node(output_node, scene);
        if (std::get<0>(desc) != "<unknown>") {
          builder->shader(std::get<0>(desc), output_node.name(), "surface");
          descriptors[output_node.name()] = desc;
        }
      }

      for (tree.links.begin(link); link!=tree.links.end(); ++link) {
        if (!link->is_valid()) {
          continue;
        }

        auto from = link->from_socket();
        auto to = link->to_socket();

        auto from_desc = descriptors[from.node().name()];
        auto to_desc = descriptors[to.node().name()];

        auto from_mapping = std::get<3>(from_desc)[from.identifier()];
        auto to_mapping = std::get<2>(to_desc)[to.identifier()];

        auto from_name = std::get<0>(from_mapping);
        auto to_name = std::get<0>(to_mapping);
        
        builder->connect(
          from_name, to_name,
          from.node().name(), to.node().name()
        );
      }
    }

    void object(
      BL::Depsgraph& graph
    , BL::ViewLayer& view_layer
    , BL::BlendData& data
    , BL::Object& object
    , scene_t& scene)
    {
      if (is_mesh(object)) {
        std::cout << "Importing: " << object.name() << std::endl;
        if(auto m = mesh(graph, data, object, scene)) {
          scene.add(m);
        }
      }
    }

    void materials(BL::Depsgraph& graph, BL::Scene& blender_scene, scene_t& scene) {
      BL::Depsgraph::ids_iterator id;
      for (graph.ids.begin(id); id!=graph.ids.end(); ++id) {
        if (is_material(*id)) {
          BL::Material material(*id);
          std::cout << "Importing material: " << material.name() << std::endl;
          if (material.use_nodes() && material.node_tree()) {
            material_t* out = new material_t();
            {
              std::unique_ptr<material_t::builder_t> builder(out->builder());

              BL::ShaderNodeTree tree(material.node_tree());
              shader_tree(tree, graph, blender_scene, builder);
            }
            scene.add(material.name(), out);
          }
          else {
            std::cout
              << "Skipping material without shader tree: "
              << material.name() << std::endl;
          }
        }
      }
    }

    void objects(BL::Depsgraph& graph, BL::BlendData& data, scene_t& scene) {
      auto layer = graph.view_layer_eval();

      BL::Depsgraph::object_instances_iterator i;
      graph.object_instances.begin(i);
      for (; i!=graph.object_instances.end(); ++i) {
        auto instance = *i;
        auto obj = instance.object();

        if (instance.show_self()) {
          object(graph, layer, data, obj, scene);
        }
      }
    }

    void world(BL::Depsgraph& graph, BL::Scene& blender_scene, scene_t& scene) {
      BL::World world = blender_scene.world();

      if (world.use_nodes() && world.node_tree()) {
        BL::ShaderNodeTree tree(world.node_tree());

        material_t* material = new material_t();
        {
          std::unique_ptr<material_t::builder_t> builder(material->builder());
          shader_tree(tree, graph, blender_scene, builder);
        }
        scene.add("world", material);
        scene.add(light_t::make_infinite(material));
      }
    }

    void camera(
      BL::RenderSettings& settings
    , BL::RenderEngine& engine
    , BL::Scene& blender_scene
    , scene_t& scene)
    {
      auto object = blender_scene.camera();

      BL::Array<float, 16> bm;
      engine.camera_model_matrix(object, false, bm);
      memcpy(&scene.camera.to_world, &bm, sizeof(float)*16);

      BL::Camera camera(object.data());

      if (camera.dof()) {
        // TODO: log unsupported config
        std::cout << "DOF object unsupported" << std::endl;
      }

      scene.camera.film.width = settings.resolution_x();
      scene.camera.film.height = settings.resolution_y();

      scene.camera.focal_length = camera.lens();
      scene.camera.fov = camera.angle();
      scene.camera.sensor_width = camera.sensor_width();
      scene.camera.sensor_height = camera.sensor_height();
    }

    void import(
      BL::RenderSettings& settings
    , BL::RenderEngine& engine
    , BL::Scene& blender_scene
    , BL::Depsgraph& graph
    , BL::BlendData& data
    , scene_t& scene)
    {
      materials(graph, blender_scene, scene);
      objects(graph, data, scene);
      world(graph, blender_scene, scene);
      camera(settings, engine, blender_scene, scene);
    }
  }
}
