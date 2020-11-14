#pragma once

#include "sampling.hpp"
#include "math/simd.hpp"
#include "math/simd/sampling.hpp"
#include "state.hpp"
#include "utils/compiler.hpp"

#include <cmath>
#include <limits>

namespace camera {
  /*
  inline Imath::V2f sample_aperture(
    const camera_t& camera
  , const Imath::V2f& sample)
  {
    // TODO implement aperture blades
    return sample::disc::concentric(sample) * camera.aperture_radius;
  }

  struct perspective_kernel_t {
    template<typename Tile, typename Samples, int N>
    inline void operator()(
        const camera_t& camera
      , const Tile& tile
      , const Samples& samples
      , ray_t<N>* rays)
    {
      const auto zoom  = 1.12f * std::tan(camera.fov * 0.5f);
      const auto ratio = (float)camera.film.width/(float)camera.film.height;

      const auto stepx = 1.0f / (float)camera.film.width;
      const auto stepy = 1.0f / (float)camera.film.height;

      auto off = 0, sample = 0;
      for (auto y=0; y<tile.h; ++y) {
        for (auto x=0; x<tile.w; x+=Samples::step, ++sample) {
          for (auto i=0; i<Samples::step; ++i) {
            const auto ndcx = -0.5f + stepx * (0.5f + x);
            const auto ndcy = stepy * (-0.5f * y) - 0.5f; 

            Imath::V3f p(0.0f, 0.0f, 0.0f);
            Imath::V3f d(samples.film[sample].x[i], samples.film[sample].y[i], 1.0f);

            d.x = (ndcx + d.x * stepx) * (ratio * zoom);
            d.y = (ndcy + d.y * stepy) * zoom;

            if (camera.aperture_radius > 0.0f) {
              const auto lens = sample_aperture(camera, samples.lens[sample].at(i));
              const auto ft = camera.focal_length / d.z;

              p = Imath::V3f(lens.x, lens.y, 0.0f);
              d = (d * ft) - p;
            }

            camera.to_world.multVecMatrix(p, p);
            camera.to_world.multDirMatrix(d, d);

            d.normalize();

            rays->reset(off, p, d);
          }
        }
      }
    }
*/

  template<typename Samples>
  inline simd::vector2v_t sample_aperture(
    const camera_t& camera
  , const Samples& samples)
  {
    // TODO implement aperture blades
    return simd::concentric_sample_disc(samples).scaled(camera.aperture_radius);
  }

  struct perspective_kernel_t {
    template<typename Tile, typename Samples, int N>
    inline void operator()(
        const camera_t& camera
      , const Tile& tile
      , const Samples& samples
      , ray_t<N>* rays)
    {
      __aligned(32) static const float onev[] = {
        -1,-1,-1,-1,-1,-1,-1,-1
      };
/*
      __aligned(32) static const float onev[] = {
        1,1,1,1,1,1,1,1
      };
*/
      __aligned(32) static const float seqv[] = {
        0,1,2,3,4,5,6,7
      };

      const auto max  = simd::floatv_t(std::numeric_limits<float>::max());

      const simd::matrix44v_t m(camera.to_world);

      const int32_t s = ray_t<N>::step;

      auto film_sample = samples.film;
      auto lens_sample = samples.lens;

      const auto zero = simd::load(0.0f);
      const auto one  = simd::load(1.0f);
      auto step   = simd::load((float) s);
      auto half   = simd::load(0.5f);
      auto nhalf  = simd::load(-0.5f);

      auto zoom = simd::load(1.12f * std::tan(camera.fov * 0.5f));

      auto px = simd::add(simd::load((float) tile.x), simd::load(seqv));
      auto py = simd::load((float) tile.y);

      auto off = 0;

      const auto stepx = simd::load(1.0f / (float)camera.film.width);
      const auto stepy = simd::load(1.0f / (float)camera.film.height);
      const auto ratio = simd::load((float)camera.film.width/(float)camera.film.height);

      auto sy = py;
      for (auto y=0; y<tile.h; ++y) {
        const auto ndcy = half - (nhalf + sy) * stepy;
        auto sx = px;

        for (auto x=0; x<tile.w; x+=s, ++film_sample, ++lens_sample, off+=s) {
          const auto ndcx = (nhalf + sx) * stepx - half;

          simd::vector3v_t p(0.0f, 0.0f, 0.0f);
          simd::vector3v_t d(film_sample->x, film_sample->y, onev);

          d.x = (ndcx + d.x * stepx) * ratio * zoom;
          d.y = (ndcy + d.y * stepy) * zoom;

          d.normalize();

          if (!camera.is_pinhole()) {
            const auto lens = sample_aperture(camera, lens_sample->stream());
            const auto ft = simd::abs(simd::load(camera.focal_distance) / d.z);

            p = simd::vector3v_t(lens.x, lens.y, zero);
            d = (d * ft) - p;
            d.normalize();
          }

          p = simd::transform_point(m, p);
          d = simd::transform_vector(m, d);
          
          rays->reset(off, p, d, max, simd::int32v_t(0));

          sx = simd::add(sx, step);
        }

        sy = simd::add(sy, one);
      }
    }
  };
}
