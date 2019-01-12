#include "xpu.hpp"
#include "xpu/cpu.hpp"

xpu_t::~xpu_t() {
}

std::vector<xpu_t*> xpu_t::discover(const parsed_options_t& options) {
  return { cpu_t::make(options) };
}
