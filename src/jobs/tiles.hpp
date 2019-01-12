#pragma once

#include <atomic>
#include <iostream>

namespace job {
  /* A job that describes a set of precomputed tiles to be rendered */
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
      auto htiles = width / tile_size;
      auto vtiles = height / tile_size;

      const auto rh = height - tile_size * vtiles;
      const auto rw = width - tile_size * htiles;

      if (rh > 0) {
        vtiles++;
      }

      if (rw > 0) {
        htiles++;
      }

      auto queue = new tiles_t(htiles*vtiles);

      for (auto y=0u; y<vtiles; ++y) {
	for (auto x=0u; x<htiles; ++x) {
          auto tw = tile_size;
          auto th = tile_size;

          if (y == vtiles-1 && rh > 0) {
            th = rh;
          }

          if (x == htiles-1 && rw > 0) {
            tw = rw;
          }

	  queue->tiles[y * htiles + x] = { x*tile_size, y*tile_size, tw, th };
	}
      }

      return queue;
    }
  };
}
