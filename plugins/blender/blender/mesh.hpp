#pragma once

#include "mesh.hpp"
#include "scene.hpp"

#include <MEM_guardedalloc.h>
#include <RNA_access.h>
#include <RNA_blender_cpp.h>
#include <RNA_types.h>

#include <mikktspace.h>

#include <functional>
#include <list>
#include <map>
#include <unordered_map>
#include <vector>

namespace blender {
  namespace import {
    mesh_t* mesh(BL::Depsgraph& graph, BL::BlendData& data, BL::Object& object, const scene_t& scene) {
      auto blender_mesh =
        data.meshes.new_from_object(object, false, graph);

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
        std::cout << "Use loop normals" << std::endl;
        
        builder->set_normals_per_vertex_per_face();
        builder->set_uvs_per_vertex_per_face();

        blender_mesh.calc_normals_split();
      }
      else {
        std::cout << "Use vertex normals" << std::endl;
      }

      uint32_t num_normals = 0;

      BL::Mesh::vertices_iterator v;
      for (blender_mesh.vertices.begin(v); v!=blender_mesh.vertices.end(); ++v) {
        const auto& a = v->co();
        builder->add_vertex(Imath::V3f(a[0], a[1], a[2]) * transform);

        if (!use_loop_normals) {
          const auto& n = v->normal();
          builder->add_normal(Imath::V3f(n[0], n[1], n[2]) * normal_transform);

          num_normals++;
        }
      }

      std::unordered_map<std::string, std::vector<uint32_t>> sets;

      uint32_t num_indices = 0;

      BL::Mesh::loop_triangles_iterator f;
      blender_mesh.loop_triangles.begin(f);
      for (auto id=0; f!=blender_mesh.loop_triangles.end(); ++f, ++id) {
        auto p = blender_mesh.polygons[f->polygon_index()];
        const auto& indices = f->vertices();
        const auto smooth = p.use_smooth() || use_loop_normals;

        builder->add_face(indices[0], indices[1], indices[2], smooth);

        auto material = blender_mesh.materials[p.material_index()];
        sets[material.name()].push_back(id);

        if (use_loop_normals) {
          const auto ns = f->split_normals();
          for (auto i=0; i<3; ++i) {
            auto n = Imath::V3f(ns[i*3], ns[i*3+1], ns[i*3+2]) *
              normal_transform;

            builder->add_normal(n);
            num_normals++;
          }
        }

        num_indices +=3;
      }

      if (use_loop_normals) {
        if (num_normals != num_indices) {
          std::cout << "Expecting to have a normal per triangle index" << std::endl;
        }
      }
      else {
        if (num_normals != num_vertices) {
          std::cout << "Expecting to have a normal per vertex" << std::endl;
        }
      }
      
      for (auto& set : sets) {
        builder->add_face_set(scene.material(set.first), set.second);
      }

      uint32_t num_uvs = 0;

      if (blender_mesh.uv_layers.length() != 0) {
        BL::Mesh::uv_layers_iterator l;
        blender_mesh.uv_layers.begin(l);
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
        if (num_uvs != num_vertices) {
          std::cout << "Expecting to have a uv coordinate per vertex" << std::endl;
        }
      }
 
      return mesh;
    }
  }
}
