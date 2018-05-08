#pragma once

#include "sampling.hpp"
#include "math/simd.hpp"
#include "state.hpp"

#include <limits>

namespace camera {
  template<typename Ray>
  inline void compute_ray(
    const simd::matrix44_t& m
  , const simd::vector3_t& sample
  , Ray* out
  , off)
  {
    // copy over position from transformation matrix
    simd::store(m.m[0][3], out->p.x + off);
    simd::store(m.m[1][3], out->p.y + off);
    simd::store(m.m[2][3], out->p.z + off);

    // transform film sample
    const auto wi  = m * sample;
    const auto max = simd::load(std::numeric_limits<float>::max());

    simd::store(wi, out->wi.x + off, out->wi.y + off, out->wi.z + off);
    simd::store(max, out->d + off);
  }
}

struct camera_kernel_t {
  template<typename Tile, typename Samples, int N>
  inline void operator()(
    const Imath::M44f& to_world
  , const Tile& tile
  , const Samples& samples
  , pipeline_state_t<N>& state)
  {
    static const float onev[] = {
      1,1,1,1,1,1,1,1
    };

    static const float seqv[] = {
      0,1,2,3,4,5,6,7
    };

    const simd::matrix44_t m(to_world);

    const auto step = pipeline_state_t<N>::step;

    auto sample = samples.v;
    auto ray    = state.rays;
    auto one    = simd::load(1);
    auto step   = simd::load(step);

    auto px = simd::load(tile.x) + simd::load(seqv);
    auto py = simd::load(tile.y);

    auto off  = 0;
    
    auto sy = py;
    for (auto y=0; y<tile.h; ++y) {
      auto sx = px;

      for (auto x=0; x<tile.w; x+=step, ++sample, off+=step) {
	simd::vector3_t s(sample->x, sample->y, onev);
	s.x += simd::add(s.x, sx);
	s.y += simd::add(s.y, sy);
	
	camera::compute_ray(m, s, ray, off);

	sx = simd::add(sx, step);
      }
      
      sy = simd::add(sy, one);
    }
  }
};
