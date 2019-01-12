#pragma once

#include <MEM_guardedalloc.h>
#include <RNA_access.h>
#include <RNA_blender_cpp.h>
#include <RNA_types.h>

namespace blender {
  struct session_t {
    struct details_t;
    details_t* details;

    static std::string path;

    void *python_thread_state;

    session_t(
      BL::RenderEngine& engine
    , BL::Preferences& userpref
    , BL::BlendData& data
    , bool preview_osl);

    session_t(
      BL::RenderEngine& engine
    , BL::Preferences& userpref
    , BL::BlendData& data
    , BL::SpaceView3D& v3d
    , BL::RegionView3D& rv3d
    , int width
    , int height);

    void reset(BL::BlendData& data, BL::Depsgraph& depsgraph);

    void render(BL::Depsgraph& depsgraph);
  };
}
