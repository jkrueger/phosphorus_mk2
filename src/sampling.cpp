#include "sampling.hpp"
#include "light.hpp"
#include "scene.hpp"
#include "math/sampling.hpp"

#include <atomic>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <random> 
#include <functional>

/* random number engine, that complies with the c++ standard */
struct xoroshiro128plus_t {
  typedef uint64_t result_type;

  static uint64_t min() {
    return std::numeric_limits<uint64_t>::min();
  }

  static uint64_t max() {
    return std::numeric_limits<uint64_t>::max();
  }

  inline uint64_t rotl (uint64_t a, int w) {
    return a << w | a >> (64-w);
  }

  uint64_t operator()() {
   static uint64_t s0 = 1451815097307991481;
   static uint64_t s1 = 5520930533486498032;

   const uint64_t result = s0 + s1;

   s1 ^= s0;
   s0 = rotl(s0, 55) ^ s1 ^ (s1 << 14);
   s1 = rotl(s1, 36);

   return result;
  }
};

struct sampler_t::details_t {
  static const uint32_t NUM_LIGHT_SAMPLE_SETS = 64;

  // xoroshiro128plus_t gen;
  std::mt19937 gen;
  std::uniform_real_distribution<float> dis;

  std::atomic<uint32_t> light_sample;

  inline details_t()
    : light_sample(0)
    , dis(0.0f, 1.0f)
  {}

  inline float to_float(uint32_t x) {
    auto bits = (x & 0x00ffffff) | 0x3f800000;
    return *((float*) &bits) - 1.0f;
  }

  inline float sample() {
    // return to_float((uint32_t) gen());
    return dis(gen);
  }

  inline Imath::V2f sample2() {
    // uint64_t x = gen();
    // return { to_float((uint32_t) x), to_float((uint32_t) (x >> 32)) };
    return { dis(gen), dis(gen) };
  }

  inline uint32_t next_light_sample_set() {
    return light_sample++ & (NUM_LIGHT_SAMPLE_SETS-1);
  }
};

sampler_t::sampler_t(parsed_options_t& options)
  : details(new details_t())
  , spp(options.samples_per_pixel)
{}

sampler_t::~sampler_t() {
  delete pixel_samples;
  delete light_samples;
  delete details;
}

void sampler_t::preprocess(const scene_t& scene) {
#ifdef aligned_alloc
  pixel_samples = (pixel_samples_t*) aligned_alloc(32, sizeof(pixel_sample_t<>) * spp);
  light_samples = (light_samples_t*) aligned_alloc(32, sizeof(light_sample_t<>) * details_t::NUM_LIGHT_SAMPLE_SETS);
#else
  posix_memalign((void**) &pixel_samples, 32, sizeof(pixel_samples_t) * spp);
  posix_memalign((void**) &light_samples, 32, sizeof(light_samples_t) * details_t::NUM_LIGHT_SAMPLE_SETS);
#endif

  const auto spd = (uint32_t) std::lroundf(std::sqrt(spp));

  Imath::V2f stratified[spp];
  sample::stratified_2d([this]() { return details->sample(); }, stratified, spd);

  for (auto i=0; i<spp; ++i) {
    for (auto j=0; j<128; ++j) {
      for (auto k=0; k<8; ++k) {
      	pixel_samples[i].film[j].x[k] = stratified[i].x;
      	pixel_samples[i].film[j].y[k] = stratified[i].y;
        pixel_samples[i].lens[j].x[k] = sample();
        pixel_samples[i].lens[j].y[k] = sample();
      }
    }
  }

  const auto nlights = scene.num_lights();

  static uint32_t stats[8] = {0,0,0,0,0,0,0,0};

  for (auto i=0; i<details_t::NUM_LIGHT_SAMPLE_SETS; i++) {
    for (auto j=0; j<light_samples_t::size/light_samples_t::step; ++j) {
      for (auto k=0; k<light_samples_t::step; ++k) {
        const auto l = std::min((uint32_t) std::floor(sample() * nlights), nlights - 1);
        const auto light = scene.light(l);

        stats[l]++;

        light_sample_t sample;
        light->sample(details->sample2(), sample);

        auto& out = light_samples[i].samples[j];
        out.p.from(k, sample.p);
        out.u[k] = sample.uv.x;
        out.v[k] = sample.uv.y;
        out.pdf[k] = sample.pdf / nlights;
        out.mesh[k] = sample.mesh;
        out.face[k] = sample.face;
      }
    }
  }

  for(auto i=0; i<8;++i) {
    std::cout << stats[i] << ", " << std::endl; 
  }
}

float sampler_t::sample() {
  return details->sample();
}

Imath::V2f sampler_t::sample2() {
  return details->sample2();
}

const sampler_t::light_samples_t& sampler_t::next_light_samples() {
  const auto set = details->next_light_sample_set();
  const auto& samples = light_samples[set];

  return samples;
}

void sampler_t::fresh_light_samples(const scene_t* scene, light_samples_t& out) {
  const auto nlights = scene->num_lights();

  for (auto j=0; j<light_samples_t::size/light_samples_t::step; ++j) {
    for (auto k=0; k<light_samples_t::step; ++k) {
      const auto l = std::min((uint32_t) std::floor(sample() * nlights), nlights - 1);
      const auto light = scene->light(l);

      light_sample_t sample;
      light->sample(details->sample2(), sample);

      auto& s = out.samples[j];
      s.p.from(k, sample.p);
      s.u[k] = sample.uv.x;
      s.v[k] = sample.uv.y;
      s.pdf[k] = sample.pdf / nlights;
      s.mesh[k] = sample.mesh;
      s.face[k] = sample.face;
    }
  }
}
