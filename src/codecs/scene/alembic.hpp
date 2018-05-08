#pragma once

#include "../../mesh.hpp"
#include "../../scene.hpp"
#include "math.hpp"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-register"
#include <Alembic/AbcCoreOgawa/All.h>
#include <Alembic/AbcGeom/All.h>
#pragma clang diagnostic pop

#include <iostream>

namespace codec {
  namespace scene {
    namespace alembic {
      namespace Abc = Alembic::Abc;
      namespace Geo = Alembic::AbcGeom;

      void import_xform(const Geo::IXform& xform, Abc::M44d& out) {
	const auto schema = xform.getSchema();

	Alembic::AbcCoreAbstract::index_t index = 0;
	Abc::ISampleSelector selector(index);

	const auto sample = schema.getValue(selector);

	for (auto i=0; i<sample.getNumOps(); ++i) {
	  const auto op = sample.getOp(i);

	  switch (op.getType()) {
	  case Geo::kMatrixOperation:
	    out = out * op.getMatrix();
	    break;
	  default:
	    // TODO: do proper logging
	    std::cerr
	      << "Unsopported xform operation: "
	      << op.getType()
	      << std::endl;
	  }
	}
      }

      void import_camera(
	const Geo::ICamera& camera
      , scene_t& scene
      , const Abc::M44d& xform)
      {
	auto schema = camera.getSchema();

	Alembic::AbcCoreAbstract::index_t index = 0;
	Abc::ISampleSelector selector(index);

	auto sample = schema.getValue(selector);
  
	const auto focal_length = sample.getFocalLength();
	// NOTE: this is how it comes out of blender. check that this holds
	// in general
	const auto sensor_width = sample.getHorizontalAperture() * 10.0f;

	// entity to world transform
	scene.camera.to_world = Imath::M44f(xform);
	// camera parameters
	scene.camera.focal_length = focal_length;
	scene.camera.sensor_width = sensor_width;
      }

      template<typename Builder>
      uint32_t import_normals(
        Builder& builder
      , const Geo::IN3fGeomParam& normalsParam
      , const Abc::ISampleSelector& selector
      , const Abc::M44d& xform)
      {
	if (normalsParam.getNumSamples() == 0) {
	  std::cerr
	    << "Mesh does not contain normal samples. Skipping"
	    << std::endl;
	  return 0;
	}

	Geo::IN3fGeomParam::Sample sample;
	normalsParam.getExpanded(sample, selector);
	
	const auto values = sample.getVals();
	
	for (auto i=0; i<values->size(); ++i) {
	  Abc::V3f n;
	  xform.multDirMatrix((*values)[i], n);
	  builder->add_normal(Abc::V3f(-n.x, n.y, -n.z).normalize());
	}

	return values->size();
      }

      template<typename Builder>
      uint32_t import_uvs(
	Builder& builder
      , const Geo::IV2fGeomParam& uvsParam
      , const Abc::ISampleSelector& selector)
      {
	if (uvsParam.getNumSamples() == 0) {
	  std::cerr
	    << "Mesh does not contain uv samples. Skipping"
	    << std::endl;
	  return 0;
	}

	Geo::IV2fGeomParam::Sample sample;
	uvsParam.getIndexed(sample, selector);
	
	const auto values  = sample.getVals();
	const auto indices = sample.getIndices();
	
	for (auto i=0; i<indices->size(); ++i) {
	  const auto& uv = (*values)[(*indices)[i]];
	  builder->add_uv(Abc::V2f(uv.x, uv.y));
	}

	return indices->size();
      }

      template<typename Builder>
      uint32_t import_indices(
	Builder& builder
      , const Abc::IInt32ArrayProperty& indicesProperty)
      {
	const auto values = indicesProperty.getValue();
	
	for (auto i=0; i<values->size(); i+=3) {
	  builder->add_face(
	    (uint32_t)(*values)[i  ]
	  , (uint32_t)(*values)[i+1]
	  , (uint32_t)(*values)[i+2]);
	}

	return values->size();
      }

      void import_faces(
        std::vector<uint32_t>& faces
      , const Abc::IInt32ArrayProperty& facesProperty)
      {
	const auto values = facesProperty.getValue();
	
	for (auto i=0; i<values->size(); ++i) {
	  faces.push_back((uint32_t)(*values)[i]);
	}
      }
      
      void import_mesh(
        const Geo::IPolyMesh& mesh
      , mesh_t* out
      , const Abc::M44d& xform)
      {
        mesh_t::builder_t::scoped_t builder(out->builder());
	
	auto schema = mesh.getSchema();
	Geo::MeshTopologyVariance ttype = schema.getTopologyVariance();

	if (ttype == Geo::kHeterogenousTopology) {
	  std::cerr
	    << "Only homogenous topologies are supported. Skipping..."
	    << std::endl;
	  return;
	}

	Alembic::AbcCoreAbstract::index_t index = 0;
	Abc::ISampleSelector selector(index);

	const auto points = schema.getPositionsProperty().getValue();
	const auto size   = points->size();

	uint32_t num_normals;
	uint32_t num_indices;
	uint32_t num_uvs;

	for (auto i=0; i<size; ++i) {
	  builder->add_vertex((*points)[i] * xform);
	}

	if (const auto indicesParam = schema.getFaceIndicesProperty()) {
	  num_indices = import_indices(builder, indicesParam);
	}

	if (const auto normalsParam = schema.getNormalsParam()) {
	  const auto normalXform = xform.inverse().transpose();
	  num_normals = import_normals(builder, normalsParam, selector, normalXform);
	}

	if (const auto uvsParam = schema.getUVsParam()) {
	  num_uvs = import_uvs(builder, uvsParam, selector);
	}

	std::vector<std::string> names;
	schema.getFaceSetNames(names);

	if (names.size() > 0) {
	  for (const auto& n : names) {
	    const auto& set = schema.getFaceSet(n);
	    
	    std::vector<uint32_t> faces;
	    import_faces(faces, set.getSchema().getFacesProperty());
	    
	    std::cout << "Importing face set: " << n << ": " << faces.size() << std::endl;
	    /*
	    const auto material = scene.material(n);
	    if (!material) {
	      std::cout << "Missing material: " << n << std::endl;
	    }
	    */
	    builder->add_face_set(/*material, */faces);
	  }
	}

	if (num_normals > 0 && num_normals != size) {
	  if (num_normals == num_indices) {
	    builder->set_normals_per_vertex_per_face();
	  }
	  else {
	    std::cerr
	      << "The number of normal coordinates does not match the number of vertices ("
	      << num_normals << "/" << size
	      << "), and also doesn't match the number of face vertices ("
	      << num_normals << "/" << num_indices << ")"
	      << std::endl;
	  }
	}
	else {
	  builder->set_normals_per_vertex();
	}

	if (num_uvs > 0 && num_uvs != size) {
	  if (num_uvs == num_indices) {
	    builder->set_uvs_per_vertex_per_face();
	  }
	  else {
	    std::cerr
	      << "The number of uv coordinates does not match the number of vertices ("
	      << num_uvs << "/" << size
	      << "), and also doesn't match the number of face vertices ("
	      << num_uvs << "/" << num_indices << ")"
	      << std::endl;
	  }
	}
	else {
	  builder->set_uvs_per_vertex();
	}
      }

      void import_object(
        Geo::IObject object
      , scene_t& scene
      , const Abc::M44d& xform);

      void decend(
	Geo::IObject object
      , scene_t& scene
      , const Abc::M44d& xform)
      {
	for (auto i=0; i<object.getNumChildren(); ++i) {
	  import_object(object.getChild(i), scene, xform);
	}
      }

      void import_object(
        Geo::IObject object
      , scene_t& scene
      , const Abc::M44d& xform)
      {	
	if (Geo::IXform::matches(object.getHeader())) {
	  Abc::M44d childXform(xform);
	  import_xform(Geo::IXform(object, Geo::kWrapExisting), childXform);
	  decend(object, scene, childXform);
	}
	else if (Geo::ICamera::matches(object.getHeader())) {
	  import_camera(Geo::ICamera(object, Geo::kWrapExisting), scene, xform);
	}
	else if (Geo::IPolyMesh::matches(object.getHeader())) {
	  mesh_t* mesh = new mesh_t();
	  import_mesh(Geo::IPolyMesh(object, Geo::kWrapExisting), mesh, xform);
	  scene.add(mesh);
	}
	else {
	  decend(object, scene, xform);
	}
      }

      void import(const std::string& path, scene_t& scene) {
	Geo::IArchive archive(Alembic::AbcCoreOgawa::ReadArchive(), path);
	Geo::IObject  object(archive);

	import_object(object, scene, Abc::M44d());
      }
    }
  }
}
