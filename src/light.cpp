#include "light.hpp"
#include "material.hpp"
#include "mesh.hpp"
#include "scene.hpp"
#include "math/sampling.hpp"

#include <algorithm>
#include <random>

struct point_light_t : public light_t::details_t {
  material_t* material;
  Imath::V3f  position;

  point_light_t(material_t* material, const Imath::V3f& position) 
    : details_t(material->id)
    , material(material)
    , position(position)
  {}

  void sample(const Imath::V2f& uv, sampler_t::light_sample_t& out) const {
    out.p    = position;
    out.uv   = {0.0f, 0.0f};
    out.area = 0.0f;
    out.pdf  = 1.0f;
    out.data = 0;
  }

  Imath::Color3f le(
    const scene_t& scene
  , const sampler_t::light_sample_t& sample
  , const Imath::V3f& wi) const
  {
    shading_result_t light;
    material->evaluate(sample.p, wi, Imath::V3f(0.0f), sample.uv, light);
    
    return light.e;
  }
};

struct distant_light_t : public light_t::details_t {
  material_t*       material;
  float             radius;
  float             ooarea;
  Imath::V3f        direction;
  orthogonal_base_t base;
  Imath::Sphere3f   bounds;

  distant_light_t(material_t* material, const Imath::V3f& direction, float angle) 
    : details_t(material->id)
    , material(material)
    , radius(tanf(angle * 0.5f))
    , ooarea(radius > 0.0f ? 1.0f / (M_PI * radius * radius) : 1.0f)
    , direction(direction)
    , base(direction)
  {}

  void preprocess(const scene_t* scene) {
    bounds = scene->bounding_sphere();
  }

  void sample(const Imath::V2f& uv, sampler_t::light_sample_t& out) const {
    Imath::V3f d = direction;

    if (radius > 0.0f) {
      auto disc = sample::disc::concentric(uv);
      d = (direction + (base.a * uv.x + base.c * uv.y)) * radius;
      d.normalize();
    }

    float cos_theta = d.dot(direction);

    out.p    = -d * 2.0f * bounds.radius;
    out.uv   = {0.0f, 0.0f};
    out.area = 0.0f;
    out.pdf  = 1.0f; // ooarea / (cos_theta * cos_theta * cos_theta);
    out.data = 0;
  }

  Imath::Color3f le(
    const scene_t& scene
  , const sampler_t::light_sample_t& sample
  , const Imath::V3f& wi) const
  {
    shading_result_t light;
    material->evaluate(sample.p, wi, Imath::V3f(0.0f), sample.uv, light);

    return light.e;
  }
};

struct rect_light_t : public light_t::details_t {
  material_t*  material;
  float width, height;
  Imath::V3f n;
  Imath::M44f  transform;

  rect_light_t(material_t* material, const Imath::M44f& transform, float width, float height) 
   : details_t(material->id)
   , material(material)
   , transform(transform)
   , width(width)
   , height(height)
  {
    n = Imath::V3f(transform[2][0], transform[2][1], transform[2][2]);
    n.normalize();
  }

  void sample(const Imath::V2f& uv, sampler_t::light_sample_t& out) const {
    out.p    = Imath::V3f(uv.x * width, 0.0, uv.y * height) * transform;
    out.uv   = uv;
    out.area = width * height;
    out.pdf  = M_PI * 4.0f; // * 1.0f / out.area;
    out.data = 0;
  }

  Imath::Color3f le(
    const scene_t& scene
  , const sampler_t::light_sample_t& sample
  , const Imath::V3f& wi) const
  {
    shading_result_t light;
    material->evaluate(sample.p, wi, n, sample.uv, light);
    
    // std::cout << "LIGHT: " << sample.light->id << " " << light.e << std::endl;

    return light.e * std::fabs(n.dot(-wi));
  }
};

struct area_light_t : public light_t::details_t {
  mesh_t* mesh;
  uint32_t set;
  std::vector<triangle_t> triangles;
  float area;
  float* cdf;

  area_light_t(mesh_t* mesh, uint32_t set)
    : mesh(mesh)
    , set(set)
    , details_t(mesh->material(set))
    , area(0.0f)
  {
    mesh->triangles(set, triangles);
  }

  ~area_light_t() {
    delete cdf;
  }

  void preprocess(const scene_t* scene) {
    // compute cdf based on the area of each triangle of
    // the face set that defines the surface of this light source
    cdf = new float[triangles.size()];

    const auto num = triangles.size();

    for (auto i=0; i<num; ++i) {
      area += triangles[i].area();
      cdf[i] = area;
    }

    for (auto i=0; i<num; ++i) {
      cdf[i] /= area;
    }
  }

  void sample(const Imath::V2f& uv, sampler_t::light_sample_t& out) const {
    const auto num = triangles.size();

    // auto i = 0;
    // while (i < (num-1) && uv.x > cdf[i]) {
    //   ++i;
    // }

    const auto i = std::min((size_t)std::floor(uv.x * num), num - 1);

    const auto one_minus_epsilon =
      1.0f-std::numeric_limits<float>::epsilon();
    const auto remapped =
      std::min(uv.x * num - i, one_minus_epsilon);

    const auto& triangle    = triangles[i];
    const auto  barycentric = triangle_t::sample({remapped, uv.y});

    out.p    = triangle.barycentric_to_point(barycentric);
    out.uv   = barycentric;
    out.pdf  = 1.0f / area;
    out.area = area;
    out.data = 0;

    set_meshid(triangle.meshid(), out);
    set_faceid(triangle.faceid(), out);
    set_matid(triangle.matid(), out);
  }

  Imath::Color3f le(
    const scene_t& scene
  , const sampler_t::light_sample_t& sample
  , const Imath::V3f& wi) const
  {
    const auto mesh     = scene.mesh(meshid(sample));
    const auto material = scene.material(matid(sample));

    Imath::V3f n;
    Imath::V2f st;
    invertible_base_t _base;

    mesh->shading_parameters({faceid(sample), sample.uv}, n, st, _base);

    shading_result_t light;
    material->evaluate(sample.p, wi, n, st, light);

    return light.e * std::fabs(n.dot(-wi));
  }

  inline void set_meshid(uint64_t meshid, sampler_t::light_sample_t& sample) const {
    sample.data |= meshid & 0x00000000000ffff;
  }

  inline void set_faceid(uint64_t faceid, sampler_t::light_sample_t& sample) const {
    sample.data |= (faceid & 0x00000000ffffffff) << 32;
  }

  inline void set_matid(uint64_t matid, sampler_t::light_sample_t& sample) const {
    sample.data |= (matid & 0x000000000000ffff) << 16;
  }

  inline uint32_t meshid(const sampler_t::light_sample_t& sample) const {
    return (uint32_t) ((sample.data & 0x00000000000ffff));
  }

  inline uint32_t matid(const sampler_t::light_sample_t& sample) const {
    return (uint32_t) ((sample.data & 0x00000000ffff0000) >> 16);
  }

  inline uint32_t faceid(const sampler_t::light_sample_t& sample) const {
    return (uint32_t) ((sample.data & 0xffffffff00000000) >> 32);
  }
};

struct infinite_light_t : public light_t::details_t {
  infinite_light_t(uint32_t matid)
    : details_t(matid)
  {}

  void sample(const Imath::V2f& uv, sampler_t::light_sample_t& out) const  {
    Imath::V3f sampled;
    sample::hemisphere::uniform(uv, sampled, out.pdf);

    out.p = sampled * 1000.0f;
    // out.mesh = matid << 16;
  }
};

light_t::light_t(type_t type, details_t* details)
  : type(type), details(details)
{}

light_t::~light_t() {
  delete details;
}

void light_t::preprocess(const scene_t* scene) {
  switch(type) {
  case DISTANT:
    static_cast<distant_light_t*>(details)->preprocess(scene);
    break;
  case AREA:
    static_cast<area_light_t*>(details)->preprocess(scene);
    break;
  default:
    // nothing to do for other light sources
    break;
  }
}

void light_t::sample(
  const Imath::V2f& uv
, sampler_t::light_sample_t& out) const
{
  out.light = this;

  switch(type) {
  case POINT:
    static_cast<const point_light_t*>(details)->sample(uv, out);
    break;
  case DISTANT:
    static_cast<const distant_light_t*>(details)->sample(uv, out);
    break;
  case RECT:
    static_cast<const rect_light_t*>(details)->sample(uv, out);
    break;
  case AREA:
    static_cast<const area_light_t*>(details)->sample(uv, out);
    break;
  case INFINITE:
    static_cast<const infinite_light_t*>(details)->sample(uv, out);
    break;
  }
}

Imath::V3f light_t::setup_shadow_ray(
  const Imath::V3f& p
, const sampler_t::light_sample_t& sample) const 
{
  switch (type) {
    case DISTANT:
    {
      // for distant light sources the incident light direction
      // is the same for every point in the scene, so we compute
      // the shadow ray direction by starting at the
      // surface point, adding the lights direction
      return sample.p;
    }
    default:
      // for all other cases the direction for the shadow ray
      // is computed by taking the difference between the surface
      // point, and the sampled point on the light source
      return sample.p - p;
  }
}

Imath::V3f light_t::le(
  const scene_t& scene
, const sampler_t::light_sample_t& sample
, const Imath::V3f& wi) const
{
  switch (type) {
  case POINT:
    return static_cast<const point_light_t*>(details)->le(scene, sample, wi);
  case DISTANT:
    return static_cast<const distant_light_t*>(details)->le(scene, sample, wi);
  case RECT:
    return static_cast<const rect_light_t*>(details)->le(scene, sample, wi);
  case AREA:
    return static_cast<const area_light_t*>(details)->le(scene, sample, wi);
  case INFINITE:
    // return static_cast<const infinite_light_t*>(details)->le(scene, sample, wi);
    break;
  }

  return Imath::V3f(0);
}

light_t* light_t::make_point(material_t* material, const Imath::V3f& p) {
  auto details = new point_light_t(material, p);
  return new light_t(POINT, details);
}

light_t* light_t::make_distant(material_t* material, const Imath::V3f& d, float a) {
  auto details = new distant_light_t(material, d, a);
  return new light_t(DISTANT, details);
}

light_t* light_t::make_rect(material_t* material, const Imath::M44f& transform, float width, float height) {
  auto details = new rect_light_t(material, transform, width, height);
  return new light_t(RECT, details);
}

light_t* light_t::make_area(mesh_t* mesh, uint32_t set) {
  auto details = new area_light_t(mesh, set);
  return new light_t(AREA, details);
}

light_t* light_t::make_infinite(const material_t* material) {
  auto details = new infinite_light_t(material->id);
  return new light_t(INFINITE, details);
}
