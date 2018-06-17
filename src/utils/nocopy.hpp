#pragma once

struct nocopy_t {
public:
  inline nocopy_t()
  {}

private:
  inline nocopy_t(const nocopy_t&)
  {}

  inline nocopy_t& operator=(const nocopy_t&)
  { return *this; }
};
