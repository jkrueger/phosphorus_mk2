#pragma once

#include "film.hpp"
#include "sampling.hpp"
#include "jobs/tiles.hpp"
#include "math/orthogonal_base.hpp"
#include "math/simd.hpp"
#include "math/soa.hpp"
#include "utils/allocator.hpp"
#include "utils/assert.hpp"
#include "utils/compiler.hpp"
#include "utils/nocopy.hpp"

#include <limits>

struct bsdf_t;

/* Global state for the rendering of one frame */
struct frame_state_t : nocopy_t {
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

/* Describes how many rays in the pipeline are still active,
 * and to which pixels the active rays belong */
struct active_t {
  static const uint32_t size = config::STREAM_SIZE;

  // number of rays active in the pipeline
  uint32_t num;
  // indices back to real pixels for each pipeline entry
  alignas(32) uint32_t index[size];

  inline active_t()
    : num(0)
  {}

  inline void reset(uint32_t base = 0) {
    num = size;
    for (auto i=0; i<size; ++i) {
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

  inline bool is_empty() const {
    return num == 0;
  }

  inline bool has_active() const {
    return num > 0;
  }
};

static const uint32_t CAMERA   = 1;
static const uint32_t SHADOW   = 2;
static const uint32_t DIFFUSE  = 4;
static const uint32_t SPECULAR = 8;
static const uint32_t GLOSSY   = 16;
static const uint32_t HIT      = 32;
static const uint32_t MASKED   = 64;

/* flags to indicate the visibility of objects to certain
 * types of rays */
enum class visibility_t : uint32_t {
  Camera   = CAMERA,
  Shadow   = SHADOW,
  Diffuse  = DIFFUSE,
  Specular = SPECULAR,
  Glossy   = GLOSSY
};

/* Models a ray as it gets processed in the rendering pipeline */
struct ray_t {
  // ray parameters
  Imath::V3f p;
  Imath::V3f wi;
  float d;

  // surface parameters at a potential hit point
  uint32_t mesh;
  uint32_t face;
  float u;
  float v;

  // flags that indicate the state of the ray in the pipeline
  uint32_t flags;

  inline void reset(
    const Imath::V3f& _p
  , const Imath::V3f& _wi
  , uint32_t _flags = 0
  , float _d = std::numeric_limits<float>::max()) {
    p = _p;
    wi = _wi;
    d = _d;
    flags = _flags;
  }

  inline void set_surface(
      uint32_t _mesh
    , uint32_t _face
    , float _u
    , float _v) {
    mesh = _mesh;
    face = _face;
    u = _u;
    v = _v;
  }

  inline Imath::V3f at(float t) const {
    return p + wi * t;
  }

  inline void miss() {
    flags &= ~HIT;
  }

  inline void hit(float _d) {
    flags |= HIT;
    d = _d;
  }

  inline void shadow() {
    flags |= SHADOW;
  }

  inline void mask() {
    flags |= MASKED;
  }

  inline void diffuse_bounce(bool b) {
    if (b) {
      flags |= DIFFUSE;
    }
    else {
      flags &= ~DIFFUSE;
    }
  }

  inline void specular_bounce(bool b) {
    if (b) {
      flags |= SPECULAR;
    }
    else {
      flags &= ~SPECULAR;
    }
  }

  inline void glossy_bounce(bool b) {
    if (b) {
      flags |= GLOSSY;
    }
    else {
      flags &= ~GLOSSY;
    }
  }

  inline bool is_hit() const {
    return (flags & HIT) == HIT;
  }

  inline bool is_masked() const {
    return (flags & MASKED) == MASKED;
  }

  inline bool is_shadow() const {
    return (flags & SHADOW) == SHADOW;
  }

  /* checks if an object is visible to this ray */
  inline bool is_visible(uint32_t vis) const {
    return !is_shadow() || (vis & static_cast<uint32_t>(visibility_t::Shadow));
  }

  inline bool is_occluded() const {
    return is_hit() || is_masked();
  }

  inline bool is_specular() const {
    return (flags & SPECULAR) == SPECULAR;
  }

  inline uint32_t meshid() const {
    return mesh & 0x0000ffff;
  }

  inline uint32_t matid() const {
    return (mesh & 0xffff0000) >> 16;
  }
};

/* A stream of rays (AOS) */
struct rays_t {
  static const uint32_t size = config::STREAM_SIZE;

  ray_t* ray;

  void allocate(allocator_t& a) {
    ray = (ray_t*) a.allocate(size * sizeof(ray_t));
    memset(ray, 0, sizeof(ray_t) * size);
  }

  inline ray_t& operator[] (uint32_t i) {
    assert(i < config::STREAM_SIZE);
    return ray[i];
  }

  inline const ray_t& operator[] (uint32_t i) const {
    assert(i < config::STREAM_SIZE);
    return ray[i];
  }

  inline void reset(
    uint32_t off
  , const simd::vector3v_t& p
  , const simd::vector3v_t& wi
  , const uint32_t flags = 0
  , float d = std::numeric_limits<float>::max())
  {
    __aligned(32) float px[SIMD_WIDTH], py[SIMD_WIDTH], pz[SIMD_WIDTH];
    p.store(px, py, pz);

    __aligned(32) float wx[SIMD_WIDTH], wy[SIMD_WIDTH], wz[SIMD_WIDTH];
    wi.store(wx, wy, wz);

    for (auto i=0; i<SIMD_WIDTH; ++i) {
      auto& r = ray[off + i];
      r.p.x = px[i];
      r.p.y = py[i];
      r.p.z = pz[i];

      r.wi.x = wx[i];
      r.wi.y = wy[i];
      r.wi.z = wz[i];

      r.d = d;
      r.flags = flags;
    }
  }
};

namespace soa {
  /* SOA stream of rays */
  struct ray_t {
    static const uint32_t size = config::STREAM_SIZE;
    static const uint32_t step = SIMD_WIDTH;

    soa::vector3_t<size> p;
    soa::vector3_t<size> wi;
    float d[size];

    ray_t(const rays_t& rays, const active_t& active) {
      for (auto i=0; i<active.num; ++i) {
        auto& ray = rays[i];
        reset(i, ray.p, ray.wi, ray.d);
      }
    }

    inline void reset(
      uint32_t i
    , const Imath::V3f& _p
    , const Imath::V3f& _wi
    , float _d = std::numeric_limits<float>::max())
    {
      p.from(i, _p);
      wi.from(i, _wi);
      d[i] = _d;
    }

    inline void set_max_distance(uint32_t i, float _d) {
      d[i] = _d;
    }

    // inline simd::vector3_t<step> at(const simd::float_t<step>& t) const {
    //   return p + wi * t;
    // }
  };
}

/**
 * A surface interaction models a point on a surface at which
 * a ray has intersected a surface. It consists of a hitpoint,
 * and a vector pointing away from the surface, along the incoming 
 * ray dirrction, as well as the surface normal. It holds additional 
 * shading properties, such as testure coordinates, light emitted
 * from the surface, and a function describing light scattering
 * at the point, which will be filled in by the material system  
 */
struct interaction_t {
  Imath::V3f p;
  Imath::V3f wi;
  Imath::V3f n;
  Imath::V2f st;

  uint32_t flags;

  bsdf_t* bsdf;
  Imath::Color3f e;

  invertible_base_t xform;

  struct mesh_t* mesh;

  inline bool is_hit() const {
    return (flags & HIT) == HIT;
  }

  inline bool is_masked() const {
    return (flags & MASKED) == MASKED;
  }

  inline bool is_shadow() const {
    return (flags & SHADOW) == SHADOW;
  }

  inline bool is_occluded() const {
    return is_hit() || is_masked();
  }

  inline bool is_specular() const {
    return (flags & SPECULAR) == SPECULAR;
  }
};

/** A stream of surface interactions (AOS path) */
struct interactions_t {
  static const uint32_t size = config::STREAM_SIZE;

  interaction_t* interaction;

  inline void allocate(allocator_t& a) {
    interaction = (interaction_t*) a.allocate(size * sizeof(interaction_t));
  }

  inline interaction_t& operator[] (uint32_t i) {
    assert(i < config::STREAM_SIZE);
    return interaction[i];
  }

  inline const interaction_t& operator[] (uint32_t i) const {
    assert(i < config::STREAM_SIZE);
    return interaction[i];
  }
};

namespace soa {
  /**
   * A stream of surface interactions (SOA path)
   */
  template<int N = config::STREAM_SIZE>
  struct interaction_t {
    static const uint32_t size = N;
    static const uint32_t step = SIMD_WIDTH;

    soa::vector3_t<N> p;
    soa::vector3_t<N> wi;
    soa::vector3_t<N> n;

    float s[N];
    float t[N];

    uint32_t flags[N];

    soa::vector3_t<N> e;
    bsdf_t* bsdf[N];

    /* a transformation from and to this points tangent space */
    invertible_base_t xform[N];

    /* copy an interaction */
    inline void from(const interaction_t* o, uint32_t from, uint32_t to) {
      p.from(to, o->p.at(from));
      wi.from(to, o->wi.at(from));
      n.from(to, o->n.at(from));
      e.from(to, o->e.at(from));
      flags[to] = o->flags[from];
      s[to] = o->s[from];
      t[to] = o->t[from];
      bsdf[to] = o->bsdf[from];
      xform[to] = o->xform[from];
      // index[to] = o->index[from];
    }
  };
}

/* Emitted by the material system to represent the result of 
 * running one shader */
struct shading_result_t {
  Imath::V3f e; // light emitted at a point
  bsdf_t* bsdf; // scattering function for a point

  inline shading_result_t()
    : e(0)
  {}
};

