cmake_minimum_required (VERSION 3.11)
project (phosphorus)

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake)

add_executable(phosphorus
  src/bsdf.cpp
  src/core.cpp
  src/light.cpp
  src/material.cpp
  src/mesh.cpp
  src/sampling.cpp
  src/scene.cpp
  src/accel/bvh.cpp
  src/codecs/scene.cpp
  src/film/file.cpp
  src/kernels/cpu/stream_bvh_kernel.cpp
  src/kernels/cpu/linear_bvh_kernel.cpp
  src/kernels/cpu/spt.hpp
  src/xpu.cpp
  src/xpu/cpu.cpp)

include_directories(
  src/
  /usr/include/OpenEXR
  /usr/local/include
  /usr/local/include/OpenEXR)

target_link_directories(phosphorus
  PUBLIC /usr/local/lib
)

target_link_libraries(phosphorus
  pthread
  IlmImf
  Half
  Iex
  OpenImageIO
  OpenImageIO_Util
  yaml-cpp
  Alembic
  oslexec
  oslcomp
  oslquery)

SET( CMAKE_CC_COMPILER "clang")
SET( CMAKE_CXX_COMPILER "clang++")
SET( CMAKE_CXX_FLAGS_RELEASE  "${CMAKE_CXX_FLAGS} -std=c++14 -O3 -march=native -fno-rtti" )
SET( CMAKE_EXE_LINKER_FLAGS "${CMAKE_CXX_LDFLAGS} -flto" )

set( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${INCLUDE_PATHS} -std=c++14 -march=native -fno-rtti -D_DEBUG" )
