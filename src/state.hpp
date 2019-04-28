#pragma once

#include "film.hpp"
#include "sampling.hpp"
#include "jobs/tiles.hpp"
#include "math/simd.hpp"
#include "math/soa.hpp"
#include "utils/assert.hpp"
#include "utils/compiler.hpp"

#include <limits>

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
  }
};

static const uint32_t HIT      = 1;
static const uint32_t MASKED   = (1 << 1);
static const uint32_t SHADOW   = (1 << 2);
static const uint32_t SPECULAR = (1 << 3);

/* A stream of rays */
template<int N = config::STREAM_SIZE>
struct ray_t {
  static const uint32_t size = N;
  static const uint32_t step = SIMD_WIDTH;

  soa::vector3_t<N> p;
  soa::vector3_t<N> wi;
  float d[N];

  uint32_t mesh[N];
  uint32_t face[N];
  float    u[N];
  float    v[N];

  uint32_t flags[N];

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

  template<int M>
  inline void reset(
    uint32_t off
  , const simd::vector3_t<M>& _p
  , const simd::vector3_t<M>& _wi
  , const simd::float_t<M>& _d
  , const simd::int32_t<M>& _flags)
  {
    _p.store(p, off);
    _wi.store(wi, off);
    _d.store(d, off);
    _flags.store((int32_t*) flags, off);
  }

  inline void set_surface(
    uint32_t i
  , uint32_t _mesh, uint32_t _face
  , float _u, float _v)
  {
    assert(i < size);

    mesh[i] = _mesh;
    face[i] = _face;
    u[i]    = _u;
    v[i]    = _v;
  }

  template<int M>
  inline void set_surface(
    uint32_t i
  , const simd::int32_t<M>& _mesh
  , const simd::int32_t<M>& _face
  , const simd::float_t<M>& _u
  , const simd::float_t<M>& _v)
  {
    assert(i < size);

    _mesh.store((int32_t*) mesh + i);
    _face.store((int32_t*) face + i);
    _u.store(u + i);
    _v.store(v + i);
  }

  inline void miss(uint32_t i) {
    flags[i] &= ~HIT;
  }

  inline void hit(uint32_t i, float _d) {
    assert(i < size);
    assert(_d < d[i]);
    flags[i] |= HIT;
    d[i] = _d;
  }

  inline void shadow(uint32_t i) {
    flags[i] |= SHADOW;
  }

  inline void mask(uint32_t i) {
    flags[i] |= MASKED;
  }

  inline void specular_bounce(uint32_t i, bool b) {
    if (b) {
      flags[i] |= SPECULAR;
    }
    else {
      flags[i] &= ~SPECULAR;
    }
  }

  inline bool is_hit(uint32_t i) const {
    return (flags[i] & HIT) == HIT;
  }

  inline bool is_masked(uint32_t i) const {
    return (flags[i] & MASKED) == MASKED;
  }

  inline bool is_shadow(uint32_t i) const {
    return (flags[i] & SHADOW) == SHADOW;
  }

  inline bool is_occluded(uint32_t i) const {
    return is_hit(i) || is_masked(i);
  }

  inline bool is_specular(uint32_t i) const {
    return (flags[i] & SPECULAR) == SPECULAR;
  }

  inline uint32_t meshid(uint32_t i) const {
    return mesh[i] & 0x0000ffff;
  }

  inline uint32_t matid(uint32_t i) const {
    return (mesh[i] & 0xffff0000) >> 16;
  }
};

/* A stream of surface interactions
 *
 * A surface interaction models a point on a surface at which
 * a ray has intersected a surface. It consists of a hitpoint,
 * and a vector pointing away from the surface, along the incoming 
 * ray dirrction, as well as the surface normal. It holds additional 
 * shading properties, such as testure coordinates, light emitted
 * from the surface, and a function describing light scattering
 * at the point, which will be filled in by the material system 
 */
template<int N = config::STREAM_SIZE>
struct interaction_t {
  static const uint32_t size = N;
  static const uint32_t step = SIMD_WIDTH;

  uint32_t index[N];

  soa::vector3_t<N> p;
  soa::vector3_t<N> wi;
  soa::vector3_t<N> n;

  float s[N];
  float t[N];

  uint32_t flags[N];

  soa::vector3_t<N> e;
  bsdf_t* bsdf[N];

  inline void from(const interaction_t* o, uint32_t from, uint32_t to) {
    p.from(to, o->p.at(from));
    wi.from(to, o->wi.at(from));
    n.from(to, o->n.at(from));
    e.from(to, o->e.at(from));
    flags[to] = o->flags[from];
    s[to] = o->s[from];
    t[to] = o->t[from];
    bsdf[to] = o->bsdf[from];
  } 

  inline bool is_hit(uint32_t i) const {
    return (flags[i] & HIT) == HIT;
  }

  inline bool is_masked(uint32_t i) const {
    return (flags[i] & MASKED) == MASKED;
  }

  inline bool is_shadow(uint32_t i) const {
    return (flags[i] & SHADOW) == SHADOW;
  }

  inline bool is_occluded(uint32_t i) const {
    return is_hit(i) || is_masked(i);
  }

  inline bool is_specular(uint32_t i) const {
    return (flags[i] & SPECULAR) == SPECULAR;
  }
};

/* Emitted by the material system to represent the result of 
 * running one shader */
struct shading_result_t {
  Imath::V3f e; // light emitted at a point
  bsdf_t* bsdf; // scattering function for a point

  inline shading_result_t()
    : e(0)
  {}
};

/* Describes how many rays in the pipeline are still active,
 * and to which pixels the active rays belong */
template<int N = 1024>
struct active_t {
  static const uint32_t size = N;

  // number of rays active in the pipeline
  uint32_t num;
  // indices back to real pixels for each pipeline entry
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
