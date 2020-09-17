#pragma once

#include <OpenEXR/ImathColor.h>
#include <OpenImageIO/imageio.h>

struct allocator_t;

struct render_buffer_t {
  static const OIIO::ustring PRIMARY;
  static const OIIO::ustring NORMALS;

  struct channel_format_t {
    OIIO::ustring name;
    uint32_t components;

    inline channel_format_t(const OIIO::ustring& name, uint32_t components)
     : name(name), components(components)
    {}
  };

  /* describes the desired output channels in a render buffer */
  struct descriptor_t {
    std::vector<channel_format_t> channels;

    /* clear all channels from the descriptor */
    void reset();

    /* request a new custom channel to be added to the render buffer */
    void request(const OIIO::ustring& name, uint32_t components);
  };

  /* a channel provides methods for accessing a specific sub set of a
   * render buffer */
  struct channel_t {
    OIIO::ustring name; // a unique name for the channel
    uint32_t components; // number of components in this buffer
    uint32_t offset; // offset to the beginning of the channel data
    render_buffer_t* buffer; // link back to the owening render buffer

    channel_t(const OIIO::ustring& name, uint32_t components, uint32_t offset, render_buffer_t* buffer) 
     : name(name)
     , components(components)
     , offset(offset)
     , buffer(buffer)
    {}

    /* set a pixel in the render buffer */
    void set(int x, int y, const Imath::V3f& c);

    /* add to a pixel in the render buffer */
    void add(int x, int y, const Imath::V3f& c);

    /* extract one pixel from the channel, and write it to out */
    const void get(uint32_t x, uint32_t y, float* out) const;

    inline uint32_t width() const { 
      return buffer->width; 

    };
    inline uint32_t height() const { 
      return buffer->height; 
    }; 

    inline uint32_t xstride() const { 
      return buffer->xstride; 
    };
    
    inline uint32_t ystride() const { 
      return buffer->ystride; 
    };

    const float* data() const;
  };

  std::vector<channel_t> channels;

  float* buffer; // pointer to the data in this render buffer

  uint32_t width;  // width of the render buffer in pixels
  uint32_t height; // height of the render buffer in pixels

  uint32_t xstride; // size of a single pixel in the buffer
  uint32_t ystride; // size of a line in the buffer

  render_buffer_t(const descriptor_t& format);

  /* get the nth channel in the buffer */
  channel_t* channel(size_t i);
  const channel_t* channel(size_t i) const;

  /* lookup a channel in the buffer by name */
  channel_t* channel(const OIIO::ustring& name);
  const channel_t* channel(const OIIO::ustring& name) const;

  void allocate(allocator_t& allocator, uint32_t width, uint32_t height);
};
