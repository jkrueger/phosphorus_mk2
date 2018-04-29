#pragma once

#include <atomic>

namespace job {

  /* A ob that describes a set of precomputed tiles to be renderer */
  struct tiles_t {
    struct tile_t {
      uint32_t x, y;
      uint32_t w, h;
    };

    uint32_t size;
    tile_t*  tiles;

    std::atomic<uint32_t> tile;

    inline tiles_t(uint32_t size)
      : size(size), tile(0)
    {
      tiles = new tile_t[size];
    }

    inline ~tiles_t() {
      delete[] tiles;
    }

    const bool next(tile_t& out) {
      const auto t = tile++;
      if (t < size) { 
	out = tiles[t];
	return true;
      }
      return false;
    }

    static tiles_t* make(uint32_t width, uint32_t height, uint32_t tile_size) {
      const auto htiles = width / tile_size;
      const auto vtiles = height / tile_size;

      auto queue = new tiles_t(htiles*vtiles);

      for (uint32_t y=0; y<height; y+=tile_size) {
	for (uint32_t x=0; x<width; x+=tile_size) {
	  queue->tiles[y * htiles + x] = { x, y, tile_size, tile_size };
	}
      }

      return queue;
    }
  };
}
