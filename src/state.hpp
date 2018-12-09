#pragma once

#include "film.hpp"
#include "sampling.hpp"
#include "jobs/tiles.hpp"
#include "math/simd.hpp"
#include "math/soa.hpp"
#include "utils/compiler.hpp"

struct bsdf_t;

/* Global state for the rendering of one frame */
struct frame_state_t {
  sampler_t*    sampler;
  job::tiles_t* tiles;
  film_t<>*     film;

  inline frame_state_t(sampler_t* sampler, job::tiles_t* tiles, film_t<>* film)
    : sampler(sampler)
    , tiles(tiles)
    , film(film)
  {}

  inline ~frame_state_t() {
    delete tiles;
  }
};

namespace details {
  static const uint32_t HIT      = 1;
  static const uint32_t MASKED   = (1 << 1);
  static const uint32_t SHADOW   = (1 << 2);
  static const uint32_t SPECULAR = (1 << 3);
}

/* a stream of rays */
template<int N = config::STREAM_SIZE>
struct ray_t {
  static const uint32_t size = N;
  static const uint32_t step = SIMD_WIDTH;

  soa::vector3_t<N> p;
  soa::vector3_t<N> wi;
  float d[N];

  uint32_t mesh[N];
  uint32_t set[N];
  uint32_t face[N];
  float    u[N];
  float    v[N];

  uint8_t  flags[N];

  // inline ray_t() {
  //   memset(flags, 0, sizeof(flags));
  // }

  inline void reset(
    uint32_t i
  , const Imath::V3f& _p
  , const Imath::V3f& _wi
  , float _d = std::numeric_limits<float>::max())
  {
    p.from(i, _p);
    wi.from(i, _wi);
    d[i] = _d;
    flags[i] = 0;
  }

  inline void set_surface(
    uint32_t i
  , uint32_t _mesh, uint32_t _set, uint32_t _face
  , float _u, float _v)
  {
    assert(i < size);

    mesh[i] = _mesh;
    set[i]  = _set;
    face[i] = _face;
    u[i]    = _u;
    v[i]    = _v;
  }

  inline void miss(uint32_t i) {
    flags[i] &= ~details::HIT;
  }

  inline void hit(uint32_t i, float _d) {
    assert(_d < d[i]);
    flags[i] |= details::HIT;
    d[i] = _d;
  }

  inline void shadow(uint32_t i) {
    flags[i] |= details::SHADOW;
  }

  inline void mask(uint32_t i) {
    flags[i] |= details::MASKED;
  }

  inline void specular_bounce(uint32_t i, bool b) {
    if (b) {
      flags[i] |= details::SPECULAR;
    }
    else {
      flags[i] &= ~details::SPECULAR;
    }
  }

  inline bool is_hit(uint32_t i) const {
    return (flags[i] & details::HIT) == details::HIT;
  }

  inline bool is_masked(uint32_t i) const {
    return (flags[i] & details::MASKED) == details::MASKED;
  }

  inline bool is_shadow(uint32_t i) const {
    return (flags[i] & details::SHADOW) == details::SHADOW;
  }

  inline bool is_occluded(uint32_t i) const {
    return is_hit(i) || is_masked(i);
  }

  inline bool is_specular(uint32_t i) const {
    return (flags[i] & details::SPECULAR) == details::SPECULAR;
  }
};

/* a stream of surface interactions */
template<int N = config::STREAM_SIZE>
struct interaction_t {
  static const uint32_t size = N;
  static const uint32_t step = SIMD_WIDTH;

  soa::vector3_t<N> p;
  soa::vector3_t<N> wi;
  soa::vector3_t<N> n;

  float s[N];
  float t[N];

  uint8_t flags[N];

  soa::vector3_t<N> e;
  bsdf_t* bsdf[N];

  inline void from(uint32_t index, const interaction_t* o) {
    p.from(index, o->p.at(index));
    wi.from(index, o->wi.at(index));
    n.from(index, o->n.at(index));
    e.from(index, o->e.at(index));
    flags[index] = o->flags[index];
    s[index] = o->s[index];
    t[index] = o->t[index];
    bsdf[index] = o->bsdf[index];
  } 

  inline bool is_hit(uint32_t i) const {
    return (flags[i] & details::HIT) == details::HIT;
  }

  inline bool is_masked(uint32_t i) const {
    return (flags[i] & details::MASKED) == details::MASKED;
  }

  inline bool is_shadow(uint32_t i) const {
    return (flags[i] & details::SHADOW) == details::SHADOW;
  }

  inline bool is_occluded(uint32_t i) const {
    return is_hit(i) || is_masked(i);
  }

  inline bool is_specular(uint32_t i) const {
    return (flags[i] & details::SPECULAR) == details::SPECULAR;
  }
};

struct shading_result_t {
  Imath::V3f e; // light emitted at a point
  bsdf_t* bsdf; // scattering function for a point

  inline shading_result_t()
    : e(0)
  {}
};

/* active masks for the pipeline and occlusion query states, 
   to track which rays are currently active */
template<int N = 1024>
struct active_t {
  uint32_t num;
  uint32_t index[N];

  inline active_t()
    : num(0)
  {}

  inline void reset(uint32_t base) {
    num = N;
    for (auto i=0; i<N; ++i) {
      index[i] = base + i;
    }
  }

  inline void add(uint32_t i) {
    index[num++] = i;
  }

  inline uint32_t clear() {
    uint32_t n = num;
    num = 0;
    return n;
  }

  inline bool has_live_paths() const {
    return num > 0;
  }
};
