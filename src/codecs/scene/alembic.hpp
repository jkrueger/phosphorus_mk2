#pragma once

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
