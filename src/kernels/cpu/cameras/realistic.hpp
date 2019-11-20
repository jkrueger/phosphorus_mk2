#pragma once

namespace camera {
  /* implements a realistic camera model with multiple lenses */
  struct realistic_t {
    template<typename Tile, typename Samples, int N>
    inline void operator()(
      const camera_t& camera
    , const Tile& tile
    , const Samples& samples
    , ray_t<N>* rays)
    {
      
    }
  };
}
