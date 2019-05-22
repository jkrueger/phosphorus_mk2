#pragma once

#include <immintrin.h>
#include <xmmintrin.h>

#include <stdint.h>

#include "float8.hpp"
#include "utils/compiler.hpp"

namespace simd {
  __forceinline __m256i load(::int32_t x) {
    return _mm256_set1_epi32(x);
  }

  __forceinline __m256i load(const ::int32_t* const x) {
    return _mm256_load_si256((__m256i * const) x);
  }

  __forceinline __m256i loadu(const ::int32_t* const x) {
    return _mm256_loadu_si256((__m256i * const) x);
  }

  __forceinline __m256i gather(const ::int32_t* const p, const __m256i& i) {
    return _mm256_i32gather_epi32(p, i, 4);
  }

  __forceinline void store(const __m256i& l, ::int32_t* r) {
    _mm256_store_si256((__m256i*) r, l);
  }

  __forceinline __m256i andnot(const __m256i& l, const __m256i& r) {
    return _mm256_castps_si256(andnot(l, r));
  }

  __forceinline __m256i add(const __m256i& l, const __m256i& r) {
    return _mm256_add_epi32(l, r);
  }

  __forceinline __m256i sub(const __m256i& l, const __m256i& r) {
    return _mm256_castps_si256(sub(l, r));
  }

  template<int N>
  struct int32_t
  {};

  template<>
  struct int32_t<8> {
    typedef __m256i type;

    // we define a proper vector type, since __m256i defaults to
    // to word size (i.e. 4 64bit integers, rather than 8 32bit)
    typedef int v8 __attribute__((vector_size (32)));

    type v;

    __forceinline int32_t()
      : v(simd::load(0))
    {}

    __forceinline int32_t(const int32_t& cpy)
      : v(cpy.v)
    {}

    __forceinline int32_t(const float_t<8>& f)
      : v(_mm256_cvtps_epi32(f.v))
    {}

    __forceinline int32_t(const type& v)
      : v(v)
    {}

    __forceinline int32_t(::int32_t i)
      : v(simd::load(i))
    {}

    __forceinline int32_t(const ::int32_t* const p)
      : v(simd::load(p))
    {}

    __forceinline int32_t(const ::int32_t* const v, const int32_t& indices)
      : v(simd::gather(v, indices.v))
    {}

    static __forceinline int32_t loadu(const ::int32_t* const p) {
      return int32_t(simd::loadu(p));
    }

    __forceinline void store(::int32_t* p) const {
      simd::store(v, p);
    }

    __forceinline void store(::int32_t* p, uint32_t off) const {
      simd::store(v, p + off);
    }

    __forceinline int32_t operator&(const int32_t& r) const {
      return int32_t(_mm256_castps_si256(_and(v, r.v)));
    }

    __forceinline int32_t operator&(const float_t<8>& r) const {
      return int32_t(_mm256_castps_si256(_and(v, r.v)));
    }

    __forceinline int32_t operator|(const int32_t& r) const {
      return int32_t(_mm256_castps_si256(_or(v, r.v)));
    }

    __forceinline int32_t operator|(const float_t<8>& r) const {
      return int32_t(_mm256_castps_si256(_or(v, r.v)));
    }

    __forceinline int32_t operator^(const int32_t& r) const {
      return int32_t(_mm256_castps_si256(_xor(v, r.v)));
    }

    __forceinline int32_t operator^(const float_t<8>& r) const {
      return int32_t(_mm256_castps_si256(_xor(v, r.v)));
    }

    __forceinline int32_t operator+(const int32_t& r) const {
      return int32_t(add(v, r.v));
    }

    __forceinline int32_t operator-(const int32_t& r) const {
      return int32_t(sub(v, r.v));
    }

    __forceinline int32_t operator*(const int32_t& r) const {
      return int32_t(mul(v, r.v));
    }

    __forceinline int32_t operator/(const int32_t& r) const {
      return int32_t(div(v, r.v));
    }

    __forceinline int32_t operator+(::int32_t r) const {
      return int32_t(add(v, simd::load(r)));
    }

    __forceinline int32_t operator-(::int32_t r) const {
      return int32_t(sub(v, simd::load(r)));
    }

    __forceinline int32_t operator== (const int32_t& r) const {
      return int32_t(eq(v, r.v));
    }

    __forceinline int32_t operator!= (const int32_t& r) const {
      return int32_t(neq(v, r.v));
    }

    __forceinline int32_t operator<= (const int32_t& r) const {
      return int32_t(lte(v, r.v));
    }

    __forceinline int32_t operator>= (const int32_t& r) const {
      return int32_t(gte(v, r.v));
    }
  };

  template<int N>
  __forceinline int32_t<N> andnot(const int32_t<N>& l, const int32_t<N>& r) {
    return int32_t<N>(andnot(l.v, r.v));
  }

  template<int N>
  __forceinline float_t<N> operator&(const float_t<N>& l, const int32_t<N>& r) {
    return float_t<N>(_and(l.v, r.v));
  }

  template<int N>
  __forceinline size_t to_mask(const int32_t<N>& i) {
    return movemask(i.v);
  }

  template<typename T, int N>
  __forceinline int32_t<N> select(
    const T& m
  , const int32_t<N>& l
  , const int32_t<N>& r)
  {
    return int32_t<N>(_mm256_castps_si256(select(m.v, l.v, r.v)));
  }
}
