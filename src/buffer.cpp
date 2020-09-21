#include "buffer.hpp"

#include "utils/allocator.hpp"

#include <algorithm>

const OIIO::ustring render_buffer_t::PRIMARY("primary");
const OIIO::ustring render_buffer_t::NORMALS("normals");

void render_buffer_t::descriptor_t::reset() {
  channels.clear();
}

void render_buffer_t::descriptor_t::request(const OIIO::ustring& name, uint32_t components) {
  channels.emplace_back(name, components);
}

void render_buffer_t::channel_t::set(int x, int y, const Imath::V3f& v) {
  const auto index = offset + y * buffer->ystride + x * buffer->xstride;
  buffer->buffer[index    ] = v.x;
  buffer->buffer[index + 1] = v.y;
  buffer->buffer[index + 2] = v.z;
};

void render_buffer_t::channel_t::add(int x, int y, const Imath::V3f& v) {
  const auto index = offset + y * buffer->ystride + x * buffer->xstride;
  buffer->buffer[index    ] += v.x;
  buffer->buffer[index + 1] += v.y;
  buffer->buffer[index + 2] += v.z;
};

const void render_buffer_t::channel_t::get(uint32_t x, uint32_t y, float* out) const {
  const auto index = offset + y * buffer->ystride + x * buffer->xstride;
  const auto from  = buffer->buffer + index;

  for (auto i=0; i<components; ++i) {
    out[i] = from[i];
  }
}

const float* render_buffer_t::channel_t::data() const {
  return buffer->buffer + offset;
}

render_buffer_t::render_buffer_t(const descriptor_t& format) 
: xstride(0), ystride(0), buffer(nullptr) {

  for (const auto& channel : format.channels) {
    channels.emplace_back(channel.name, channel.components, xstride, this);
    xstride += channel.components;
  }
}

render_buffer_t::channel_t* render_buffer_t::channel(size_t c) {
  return &channels[c];
}

const render_buffer_t::channel_t* render_buffer_t::channel(size_t c) const {
  return &channels[c];
}

render_buffer_t::channel_t* render_buffer_t::channel(const OIIO::ustring& name) {
  const auto guard = std::find_if(channels.begin(), channels.end(), [&](auto x) {
    return x.name == name;
  });

  if (guard == channels.end()) {
    return nullptr;
  }

  return &(*guard);
}

const render_buffer_t::channel_t* render_buffer_t::channel(const OIIO::ustring& name) const {
  const auto guard = std::find_if(channels.begin(), channels.end(), [&](auto x) {
    return x.name == name;
  });

  if (guard == channels.end()) {
    return nullptr;
  }

  return &(*guard);
}

void render_buffer_t::allocate(allocator_t& allocator, uint32_t _width, uint32_t _height) {
  width = _width;
  height = _height;
  ystride = xstride * width;

  const auto size = height * ystride * sizeof(float);

  buffer = (float*) allocator.allocate(size);
  memset(buffer, 0, size);
}
