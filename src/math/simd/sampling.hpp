#pragma once

#include "float8.hpp"
#include "vector.hpp"

namespace simd {
  template<int N>
  inline vector2_t<N> concentric_sample_disc(const vector2_t<N>& samples) {
    static const float_t<N> pi_o_2(2.0f / M_PI);
    static const float_t<N> pi_o_4(4.0f / M_PI);

    const auto offset = floatv_t(2.0f) * samples - vector2_t<N>(1, 1);

    const auto x_gt_y = abs(samples.x) > abs(samples.y);
    const auto r = select(x_gt_y, samples.x, samples.y);

    const auto theta1 = pi_o_4 * (samples.y / samples.x);
    const auto theta2 = pi_o_2 - pi_o_4 * (samples.x / samples.y);
    const auto theta = select(x_gt_y, theta1, theta2);

    // compiler will generate a vectorized version here, which isn't 
    // available as intrinsics
    float thetav[N], sin[N], cos[N];
    theta.store(thetav);

    for (auto i=0; i<N; ++i) {
      sin[i] = std::sin(thetav[i]);
      cos[i] = std::cos(thetav[i]);
    }

    return r * vector2_t<N>(cos, sin);
  }
}
