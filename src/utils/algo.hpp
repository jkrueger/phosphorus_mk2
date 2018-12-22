#pragma once

template<typename T>
inline constexpr const T& clamp(const T& v, const T& lo, const T& hi) {
  return std::max(lo, std::min(v, hi));
}
