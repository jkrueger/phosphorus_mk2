#pragma once

#include "sampling.hpp"
#include "math/simd.hpp"
#include "state.hpp"

#include <cmath>
#include <limits>

namespace camera {
  template<typename Ray>
  inline void compute_ray(
    const simd::vector3_t& pos
  , const simd::matrix44_t& m
  , const simd::vector3_t& sample
  , Ray out
  , uint32_t off)
  {
    // store position in rendering state
    simd::store(pos.x, out->p.x + off);
    simd::store(pos.y, out->p.y + off);
    simd::store(pos.z, out->p.z + off);

    // transform film sample
    const auto max = simd::load(std::numeric_limits<float>::max());

    auto wi = m * sample;
    wi.normalize();

    simd::store(wi, out->wi.x + off, out->wi.y + off, out->wi.z + off);
    simd::store(max, out->d + off);
  }
}

struct camera_kernel_t {
  template<typename Tile, typename Samples, int N>
  inline void operator()(
    const camera_t& camera
  , const Tile& tile
  , const Samples& samples
  , ray_t<N>* rays)
  {
    static const float onev[] = {
      -1,-1,-1,-1,-1,-1,-1,-1
    };

    static const float seqv[] = {
      0,1,2,3,4,5,6,7
    };

    const simd::vector3_t pos(Imath::V3f(0,0,0) * camera.to_world);
    const simd::matrix44_t m(camera.to_world);

    const int32_t s = ray_t<N>::step;

    auto sample = samples.v;
    auto ray    = rays;
    auto one    = simd::load(1.0f);
    auto step   = simd::load((float) s);
    auto half   = simd::load(0.5f);
    auto nhalf  = simd::load(-0.5f);

    auto fov  = 2.0f * std::atan2(camera.sensor_width * 0.5f, camera.focal_length);
    auto zoom = simd::load(std::tan(fov * 0.5f));

    auto px = simd::add(simd::load((float) tile.x), simd::load(seqv));
    auto py = simd::load((float) tile.y);

    auto off = 0;

    const auto stepx = simd::div(simd::load(1.0f), simd::load((float)camera.film.width));
    const auto stepy = simd::div(simd::load(1.0f), simd::load((float)camera.film.height));
    const auto ratio = simd::load((float)camera.film.width/(float)camera.film.height);

    auto sy = py;
    for (auto y=0; y<tile.h; ++y) {
      const auto ndcy = simd::sub(one, simd::mul(simd::load(2.0f), simd::mul(simd::add(nhalf, sy), stepy)));
      auto sx = px;

      for (auto x=0; x<tile.w; x+=s, ++sample, off+=s) {
	// const auto ndcx = simd::add(nhalf, simd::mul(sx, stepx));
	const auto ndcx = simd::sub(simd::mul(simd::load(2.0f), simd::mul(simd::add(nhalf, sx), stepx)), one);

	simd::vector3_t wi(
	  sample->x
	, sample->y
	, onev);

	wi.x = simd::mul(simd::add(ndcx, simd::mul(wi.x, stepx)), simd::mul(ratio, zoom));
	wi.y = simd::mul(simd::add(ndcy, simd::mul(wi.y, stepy)), zoom);
	
	camera::compute_ray(pos, m, wi, ray, off);

	sx = simd::add(sx, step);
      }

      sy = simd::add(sy, one);
    }
  }
};
