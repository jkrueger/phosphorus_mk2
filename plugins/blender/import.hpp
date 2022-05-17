#pragma once

#include "entities/camera.hpp"
#include "light.hpp"
#include "mesh.hpp"
#include "scene.hpp"

#include "blender/shader.hpp"

#include <MEM_guardedalloc.h>
#include <RNA_access.h>
#include <RNA_blender_cpp.h>
#include <RNA_types.h>

#include <mikktspace.h>

#include <cmath>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace blender {
  bool is_mesh(BL::Object& object) {
    auto data = object.data();
    return data && data.is_a(&RNA_Mesh);
  }

  bool is_light(BL::Object& object) {
    BL::ID id = object.data();
    return id && id.is_a(&RNA_Light);
  }

  bool is_material(BL::ID& id) {
    return id && id.is_a(&RNA_Material);
  }

  inline Imath::V3f get_row(const Imath::M44f& m, int n) {
    return Imath::V3f(m[n][0], m[n][1], m[n][2]);
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

      if (util::has_subdivision(object)) {
        std::cout << "Skipping mesh. Subdivision surfaces not supported" << std::endl;
        return nullptr;
      }

      if (blender_mesh.use_auto_smooth()) {
        BL::Depsgraph depsgraph(PointerRNA_NULL);
        blender_mesh = object.to_mesh(false, depsgraph);
      }

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
          const auto& blender_normal = v->normal();

          Imath::V3f world_space_normal;
          normal_transform.multDirMatrix(Imath::V3f(blender_normal[0], blender_normal[1], blender_normal[2]), world_space_normal);

          builder->add_normal(world_space_normal.normalize());

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
            Imath::V3f object_space_normal(ns[i*3], ns[i*3+1], ns[i*3+2]);
            Imath::V3f world_space_normal;

            normal_transform.multDirMatrix(object_space_normal, world_space_normal);
            world_space_normal.normalize();

            builder->set_normal(indices[i], world_space_normal);
            // num_normals++;
          }
        }

        builder->add_face(indices[0], indices[1], indices[2], smooth);
        num_indices +=3;
      }

      if (mesh->has_per_vertex_normals()) {
        if (num_normals != num_vertices) {
          std::cout << "Expecting to have a normal per vertex" << std::endl;
        }
      }
      else {
        if (num_normals != num_indices) {
          std::cout 
            << "Expecting to have a normal per triangle index: " 
            << num_normals
            << " != "
            << num_indices
            << std::endl;
        }
      }

      bool generate_uvs = false;
      bool generate_tangents = false;

      for (auto& set : sets) {
        const auto material = scene.material(set.first);

        if (material->has_attribute("geom:tangent")) {
          generate_tangents = true;
        }

        if (material->has_attribute("geom:uv")) {
          generate_uvs = true;
        }

        builder->add_face_set(material, set.second);
      }

      uint32_t num_uvs = 0;

      if (generate_uvs && blender_mesh.uv_layers.length() != 0) {
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

        if (mesh->has_per_vertex_uvs()) {
          if (num_uvs != num_vertices) {
            std::cout 
              << "Expecting to have a uv coordinate per vertex: " 
              << num_uvs << " != " << num_vertices 
              << std::endl;
          }
        }
        else {
          if (num_uvs != num_indices) {
            std::cout << "Expecting to have a uv coordinate per triangle index" << std::endl;
          }
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

    void shader_tree(
      BL::ShaderNodeTree& tree
    , BL::Depsgraph& graph
    , BL::Scene& scene
    , material_t::builder_t::scoped_t& builder)
    {
      std::unordered_map<std::string, const shader::compiler_t*> compilers;

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
          if (const auto compiler = shader::compiler_t::factory(node)) {
            compiler->compile(node, builder, scene);
            compilers.insert({node.name(), compiler});
          }
        }
      }

      if (auto output_node = tree.get_output_node(2)) {
        if (const auto compiler = shader::compiler_t::factory(output_node)) {
          compiler->compile(output_node, builder, scene);
          compilers.insert({output_node.name(), compiler});
        }
      }

      for (tree.links.begin(link); link!=tree.links.end(); ++link) {
        if (!link->is_valid()) {
          continue;
        }

        auto from_socket = link->from_socket();
        auto to_socket = link->to_socket();

        auto from_compiler = compilers.find(from_socket.node().name());
        auto to_compiler = compilers.find(to_socket.node().name());

        if (from_compiler == compilers.end() || to_compiler == compilers.end()) {
          std::cout 
            << "Can't find compilers for nodes: (" 
            << from_socket.node().name() << ", " 
            << to_socket.node().name() << ")" 
            << std::endl;
          continue;
        }
        
        const auto outputSocket = from_compiler->second->output_socket(from_socket);
        const auto inputSockets = to_compiler->second->input_socket(to_socket);

        if (!outputSocket) {
          std::cout 
            << "Can't find output socket \'" 
            << from_socket.identifier() 
            << "\' for shader: " 
            << link->from_socket().node().name() 
            << std::endl;
          continue;
        }

        if (inputSockets.empty()) {
          std::cout 
            << "Can't find input socket \'" 
            << to_socket.identifier() 
            << "\' for shader: " 
            << link->to_socket().node().name() 
            << std::endl;
          continue;
        }

        for (auto& socket : inputSockets) {
          builder->connect(
            outputSocket.value().name(), socket.name(),
            outputSocket.value().node(), socket.node()
          );
        }
      }
    }

    material_t* light_shader(BL::Scene& blender_scene, BL::Depsgraph& graph, BL::Light& blender_light) {
      material_t* out = new material_t(blender_light.name().c_str());

      if (blender_light.use_nodes() && blender_light.node_tree()) {
        std::unique_ptr<material_t::builder_t> builder(out->builder());

        BL::ShaderNodeTree node_tree(blender_light.node_tree());
        shader_tree(node_tree, graph, blender_scene, builder);
      }
      else {
        std::unique_ptr<material_t::builder_t> builder(out->builder());

        const auto color = blender_light.color();

        builder->parameter("Cs", Imath::Color3f(color[0], color[1], color[2]));
        builder->parameter("power", BL::PointLight(blender_light).energy());
        builder->shader("diffuse_emitter_node", "light", "surface");
        builder->shader("material_node", "out", "surface");

        builder->connect("Cout", "Cs", "light", "out");
      }

      return out;
    }

    light_t* light(BL::Scene& blender_scene, BL::Depsgraph& graph, BL::BlendData& data, BL::Object& object, scene_t& scene) {
      BL::Light blender_light(object.data());

      const auto om = object.matrix_world();

      Imath::M44f to_world;
      memcpy(&to_world, &om, sizeof(float)*16);

      switch(blender_light.type()) {
        case BL::Light::type_POINT:
        {
          // BL::PointLight blender_point_light(blender_light);

          auto material = light_shader(blender_scene, graph, blender_light);
          scene.add(material->name, material);
          scene.add(light_t::make_point(material, get_row(to_world, 3)));
          break;
        }
        case BL::Light::type_AREA:
        {
          BL::AreaLight blender_area_light(blender_light);

          auto dir = -get_row(to_world, 2);
          dir.normalize();

          auto material = light_shader(blender_scene, graph, blender_light);
          scene.add(material->name, material);

          switch (blender_area_light.shape()) {
            case BL::AreaLight::shape_SQUARE:
              return light_t::make_rect(material, to_world, blender_area_light.size(), blender_area_light.size());
            case BL::AreaLight::shape_RECTANGLE:
              return light_t::make_rect(material, to_world, blender_area_light.size(), blender_area_light.size_y());
            default:
              std::cout << "\tUnsupported area light shape: " << blender_area_light.shape() << std::endl;
          }
          break;
        }
        case BL::Light::type_SUN:
        {
          BL::SunLight blender_sun_light(blender_light);

          auto material = light_shader(blender_scene, graph, blender_light);
          scene.add(material->name, material);

          Imath::V3f dir = -get_row(to_world, 2);
          std::cout << "SUN: " << dir << std::endl;

          scene.add(light_t::make_distant(material, dir));
          break;
        }
        default:
          std::cout << "\tUnsupported light type: " << blender_light.type() << std::endl;
      }

      return nullptr;
    }

    void object(
      BL::Scene& blender_scene
    , BL::Depsgraph& graph
    , BL::ViewLayer& view_layer
    , BL::BlendData& data
    , BL::Object& object
    , scene_t& scene)
    {
      if (is_mesh(object)) {
        std::cout << "Importing mesh: " << object.name() << std::endl;
        if(auto m = mesh(graph, data, object, scene)) {
          scene.add(m);
        }
      }
      else if (is_light(object)) {
        std::cout << "Importing light: " << object.name() << std::endl;
        if (auto l = light(blender_scene, graph, data, object, scene)) {
          scene.add(l);
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
            material_t* out = new material_t(material.name());
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

    void objects(BL::Scene& blender_scene, BL::Depsgraph& graph, BL::BlendData& data, scene_t& scene) {
      auto layer = graph.view_layer_eval();

      BL::Depsgraph::object_instances_iterator i;
      graph.object_instances.begin(i);
      for (; i!=graph.object_instances.end(); ++i) {
        auto instance = *i;
        auto obj = instance.object();

        if (instance.show_self()) {
          object(blender_scene, graph, layer, data, obj, scene);
        }
      }
    }

    void world(BL::Depsgraph& graph, BL::Scene& blender_scene, scene_t& scene) {
      BL::World world = blender_scene.world();

      if (world.use_nodes() && world.node_tree()) {
        BL::ShaderNodeTree tree(world.node_tree());

        material_t* material = new material_t("world");
        {
          std::unique_ptr<material_t::builder_t> builder(material->builder());
          shader_tree(tree, graph, blender_scene, builder);
        }
        scene.add("world", material);
        scene.add(light_t::make_infinite(material));
      }
    }

    inline float camera_focal_distance(
      BL::RenderEngine &engine,
      BL::Camera &camera,
      scene_t& scene)
    {
      BL::Object dof_object = camera.dof().focus_object();

      if (!dof_object) {
        return camera.dof().focus_distance();
      }

      const auto bm = dof_object.matrix_world();

      Imath::M44f dof_to_world;
      memcpy(&dof_to_world, &bm, sizeof(float)*16);

      Imath::V3f view_dir = get_row(scene.camera.to_world, 2);
      view_dir.normalize();

      Imath::V3f dof_dir = get_row(scene.camera.to_world, 3) - get_row(dof_to_world, 3);

      return std::fabs(view_dir.dot(dof_dir));
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

      scene.camera.film.width = settings.resolution_x();
      scene.camera.film.height = settings.resolution_y();
      scene.camera.focal_length = camera.lens();
      scene.camera.fov = camera.angle();
      scene.camera.sensor_width = camera.sensor_width();
      scene.camera.sensor_height = camera.sensor_height();

      if (camera.dof() && camera.dof().use_dof()) {
        float fstop = camera.dof().aperture_fstop();
        fstop = std::max(fstop, 1e-5f);

        scene.camera.aperture_radius = (camera.lens() * 1e-3f) / (2.0f * fstop);
        scene.camera.focal_distance = camera_focal_distance(engine, camera, scene);
      }
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
      objects(blender_scene, graph, data, scene);
      world(graph, blender_scene, scene);
      camera(settings, engine, blender_scene, scene);
    }
  }
}
