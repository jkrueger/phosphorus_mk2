#pragma once

#include "../session.hpp"

#include <MEM_guardedalloc.h>
#include <RNA_access.h>
#include <RNA_blender_cpp.h>
#include <RNA_types.h>

#include <OpenImageIO/imagebufalgo.h>

#include <filesystem>

extern "C" {
  void BKE_image_user_frame_calc(void *iuser, int cfra);
  void BKE_image_user_file_path(void *iuser, void *ima, char *path);
  unsigned char *BKE_image_get_pixels_for_frame(void* image, int frame, int title);
  float *BKE_image_get_float_pixels_for_frame(void *image, int frame, int tile);
}

namespace blender {

  /* Texture management helper functions */
  namespace texture {

    namespace fs = std::filesystem;

    /* Get the file path of a blender texture, or construct a placeholder 
     * name artificially */
    static std::string image_file_path(
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

      return filepath;
    }

    /* Get an OpenImageIO buffer type, based on a blender image */
    static OIIO::TypeDesc to_oiio_buffer_type(BL::Image& image) {
      return image.is_float() ? OIIO::TypeFloat : OIIO::TypeUInt8;
    }

    /* Load pixels from a blender image source */
    static void* load_pixels(BL::Image& image) {
      if (image.is_float()) {
        return ((void*) BKE_image_get_float_pixels_for_frame(image.ptr.data, 0, 0));
      }

      return ((void*) BKE_image_get_pixels_for_frame(image.ptr.data, 0, 0));
    }

    /* Adds a texture from a file to the material system. this will create a temporary file
     * on disk, that is optimized for lookup, and colorspace converted. returns the file system path
     * to the texture */
    static std::string make(const std::string& path, const std::string& colorspace) {
      std::cout << "FROM FILE" << std::endl;

      const auto filename = fs::path(colorspace + "_" + path).stem().replace_extension(".tiff");
      const auto output = fs::temp_directory_path() / filename;

      OIIO::ImageBuf image(path);
      OIIO::ImageSpec config;
      config.attribute("maketx:incolorspace", colorspace);
      config.attribute("maketx:outcolorspace", "scene_linear");
      config.attribute("maketx:filtername", "lanczos3");
      config.attribute("maketx:colorconfig", blender::session_t::resources + "/2.90/datafiles/colormanagement/config.ocio");

      std::stringstream ss;

      if (!OIIO::ImageBufAlgo::make_texture(OIIO::ImageBufAlgo::MakeTxTexture, image, output.string(), config, &ss)) {
        throw std::runtime_error("Failed to make texture: " + ss.str());
      }

      return output.string();
    }

    /* Adds a texture from a buffer to the material system. this will create a temporary file
     * on disk, that is optimized for lookup, and colorspace converted. returns the file system path
     * of the texture */
    static std::string make(
        const std::string& name
      , void* pixels
      , uint32_t width, uint32_t height, uint32_t channels
      , OIIO::TypeDesc type
      , const std::string& colorspace) 
    {
      std::cout << "FROM BUFFER" << std::endl;

      const auto filename = fs::path(colorspace + "_" + name).stem().replace_extension(".tiff");
      const auto output = fs::temp_directory_path() / filename;

      OIIO::ImageSpec config(width, height, channels, type);
      config.attribute("maketx:incolorspace", colorspace);
      config.attribute("maketx:outcolorspace", "scene_linear");
      config.attribute("maketx:filtername", "lanczos3");
      config.attribute("maketx:colorconfig", blender::session_t::resources + "/2.90/datafiles/colormanagement/config.ocio");

      OIIO::ImageBuf image(config, pixels);

      std::stringstream ss;

      if (!OIIO::ImageBufAlgo::make_texture(OIIO::ImageBufAlgo::MakeTxTexture, image, output.string(), config, &ss)) {
        throw std::runtime_error("Failed to make texture: " + ss.str());
      }

      return output.string();
    }
  }
}
