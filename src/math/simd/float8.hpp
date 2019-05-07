#pragma once

#include <immintrin.h>
#include <xmmintrin.h>

namespace simd {
  inline __m256 load(float x) {
    return _mm256_set1_ps(x);
  }

  inline __m256 load(const float* const x) {
    return _mm256_load_ps(x);
  }

  inline __m256 loadu(const float* const x) {
    return _mm256_loadu_ps(x);
  }
  
  inline __m256 gather(const float* const p, const __m256i& i) {
    return _mm256_i32gather_ps(p, i, 4);
  }

  inline void store(const __m256& l, float* r) {
    _mm256_store_ps(r, l);
  }

  inline void storeu(const __m256& l, float* r) {
    _mm256_storeu_ps(r, l);
  }

  inline __m256 msub(const __m256& a, const __m256& b, const __m256& c) {
    return _mm256_fmsub_ps(a, b, c);
  }

  inline __m256 madd(const __m256& a, const __m256& b, const __m256& c) {
    return _mm256_fmadd_ps(a, b, c);
  }

  inline __m256 add(const __m256& l, const __m256& r) {
    return _mm256_add_ps(l, r);
  }

  inline __m256 sub(const __m256& l, const __m256& r) {
    return _mm256_sub_ps(l, r);
  }
  
  inline __m256 mul(const __m256& l, const __m256& r) {
    return _mm256_mul_ps(l, r);
  }

  inline __m256 div(const __m256& l, const __m256& r) {
    return _mm256_div_ps(l, r);
  }

  inline __m256 min(const __m256& l, const __m256& r) {
    return _mm256_min_ps(l, r);
  }

  inline __m256 max(const __m256& l, const __m256& r) {
    return _mm256_max_ps(l, r);
  }

  inline __m256 eq(const __m256& l, const __m256& r) {
    return _mm256_cmp_ps(l, r, _CMP_EQ_OS);
  }

  inline __m256 lt(const __m256& l, const __m256& r) {
    return _mm256_cmp_ps(l, r, _CMP_LT_OS);
  }

  inline __m256 lte(const __m256& l, const __m256& r) {
    return _mm256_cmp_ps(l, r, _CMP_LE_OS);
  }

  inline __m256 gt(const __m256& l, const __m256& r) {
    return _mm256_cmp_ps(l, r, _CMP_GT_OS);
  }

  inline __m256 gte(const __m256& l, const __m256& r) {
    return _mm256_cmp_ps(l, r, _CMP_GE_OS);
  }

  inline __m256 _or(const __m256& l, const __m256& r) {
    return _mm256_or_ps(l, r);
  }

  inline __m256 _and(const __m256& l, const __m256& r) {
    return _mm256_and_ps(l, r);
  }

  inline __m256 _xor(const __m256& l, const __m256& r) {
    return _mm256_xor_ps(l, r);
  }

  inline __m256 andnot(const __m256& l, const __m256& r) {
    return _mm256_andnot_ps(l, r);
  }

  inline __m256 select(const __m256& m, const __m256& l, const __m256& r) {
    return _mm256_blendv_ps(l, r, m);
  }

  inline size_t movemask(const __m256& mask) {
    return _mm256_movemask_ps(mask);
  }

  inline __m256 rcp(const __m256& x) {
    return _mm256_rcp_ps(x);
  }
  
  inline __m256 floor(const __m256& x) {
    return _mm256_floor_ps(x);
  }

  inline __m256 sqrt(const __m256& x) {
    return _mm256_sqrt_ps(x);
  }

  template<int N>
  struct float_t
  {};

  template<>
  struct float_t<8> {
    typedef __m256 type;

    type v;

    float_t()
      : v(simd::load(0.0f))
    {}

    float_t(const float_t& cpy)
      : v(cpy.v)
    {}

    float_t(const type& v)
      : v(v)
    {}

    float_t(float f)
      : v(simd::load(f))
    {}

    float_t(const float* const p)
      : v(simd::load(p))
    {}

    template<typename I>
    inline float_t(const float* const v, const I& indices)
      : v(simd::gather(v, indices.v))
    {}

    inline void store(float* p) const {
      simd::store(v, p);
    }

    inline void store(float* p, uint32_t off) const {
      simd::store(v, p + off);
    }

    inline void storeu(float* p, uint32_t off) const {
      simd::storeu(v, p + off);
    }
    
    inline float_t operator&(const float_t& r) const {
      return float_t(_and(v, r.v));
    }

    inline float_t operator|(const float_t& r) const {
      return float_t(_or(v, r.v));
    }

    inline float_t operator<(const float_t& r) const {
      return float_t(lt(v, r.v));
    }

    inline float_t operator> (const float_t& r) const {
      return float_t(gt(v, r.v));
    }

    inline float_t operator<= (const float_t& r) const {
      return float_t(lte(v, r.v));
    }

    inline float_t operator>= (const float_t& r) const {
      return float_t(gte(v, r.v));
    }

    inline float_t operator+ (const float_t& r) const {
      return float_t(add(v, r.v));
    }

    inline float_t operator- (const float_t& r) const {
      return float_t(sub(v, r.v));
    }

    inline float_t operator* (const float_t& r) const {
      return float_t(mul(v, r.v));
    }

    inline float_t operator/ (const float_t& r) const {
      return float_t(div(v, r.v));
    }

    static inline __m256 gather(const float* const p, const __m256i& i) {
      return _mm256_i32gather_ps(p, i, 4);
    }

    static inline __m256 gather(
      const float* const p
    , const __m256&  s
    , const __m256i& i
    , const __m256i& m)
    {
      return _mm256_mask_i32gather_ps(s, p, i, m, 4);
    }
  };

  typedef float_t<8> float8_t;

  template<int N>
  inline float_t<N> andnot(const float_t<N>& l, const float_t<N>& r) {
    return float_t<N>(andnot(l.v, r.v));
  }

  template<int N>
  inline float_t<N> min(const float_t<N>& l, const float_t<N>& r) {
    return float_t<N>(min(l.v, r.v));
  }

  template<int N>
  inline float_t<N> max(const float_t<N>& l, const float_t<N>& r) {
    return float_t<N>(max(l.v, r.v));
  }

  template<int N>
  inline float_t<N> floor(const float_t<N>& f) {
    return float_t<N>(floor(f.v));
  }

  template<int N>
  inline float_t<N> sqrt(const float_t<N>& f) {
    return float_t<N>(sqrt(f.v));
  }

  template<int N>
  inline size_t to_mask(const float_t<N>& f) {
    return movemask(f.v);
  }

  template<int N>
  inline float_t<N> select(
    const float_t<N>& m
  , const float_t<N>& l
  , const float_t<N>& r)
  {
    return float_t<N>(select(m.v, l.v, r.v));
  }
}
