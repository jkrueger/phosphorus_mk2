#pragma once

#include <string>

namespace fs {
  inline std::string basepath(const std::string& path) {
    int index = path.find_last_of('/');
    if (index != -1) {
      return path.substr(0, index);
    }
    return "";
  }

  inline std::string extension(const std::string& path) {
    int index = path.find_last_of('.');
    if (index != -1) {
      return path.substr(index);
    }
    return "";
  }
}
