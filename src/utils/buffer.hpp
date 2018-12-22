#pragma once

template<int N>
struct buffer_t {
  uint8_t buffer[N];
  uint8_t* pos;

  buffer_t()
    : pos(buffer)
  {}

  float* write_float(float v) {
    float* p = (float*) pos;
    (*p) = v;
    pos += sizeof(float);
    return p;
  }

  int* write_int(int v) {
    int* p = (int*) pos;
    (*p) = v;
    pos += sizeof(int);
    return p;
  }

  float* write_3f(float a, float b, float c) {
    float* p = (float*) pos;
    p[0] = a;
    p[1] = b;
    p[2] = c;
    pos += sizeof(float) * 3;
    return p;
  }

  template<typename T>
  void* write_ptr(T* ptr) {
    intptr_t* p = (intptr_t*) pos;
    (*p) = (intptr_t) ptr;
    pos += sizeof(intptr_t);
    return p;
  }
};
