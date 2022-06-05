#pragma once

#include "assert.hpp"

struct buffer_t {
  uint8_t* buffer;
  uint8_t* pos;

  size_t size;

  buffer_t(size_t s)
   : size(s)
  {
    buffer = (uint8_t*) malloc(size);
    pos = buffer;
  }

  ~buffer_t() {
    free(buffer);
  }

  float* write_float(float v) {
    if ((pos - buffer) + sizeof(float) >= size) {
      grow();
    }
  
    float* p = (float*) pos;
    (*p) = v;
    pos += sizeof(float);
    return p;
  }

  int* write_int(int v) {
    if ((pos - buffer)  + sizeof(int) >= size) {
      grow();
    }

    int* p = (int*) pos;
    (*p) = v;
    pos += sizeof(int);
    return p;
  }

  float* write_3f(float a, float b, float c) {
    if ((pos - buffer) +  (sizeof(float) * 3) >= size) {
      grow();
    }

    float* p = (float*) pos;
    p[0] = a;
    p[1] = b;
    p[2] = c;
    pos += sizeof(float) * 3;
    return p;
  }

  template<typename T>
  void* write_ptr(T* ptr) {
    if ((pos - buffer) + sizeof(intptr_t) >= size) {
      grow();
    }

    intptr_t* p = (intptr_t*) pos;
    (*p) = (intptr_t) ptr;
    pos += sizeof(intptr_t);
    return p;
  }

  void grow() {
    buffer = (uint8_t*) realloc(buffer, size * 2);
    pos = buffer + size;
    size *= 2;
  }
};
