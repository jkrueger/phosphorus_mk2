cmake_minimum_required (VERSION 3.11)
project (blender)

set( CMAKE_CXX_STANDARD 17)
set( CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(_phosphoros SHARED
  blender.cpp
  session.cpp
  sink.cpp
  ./blender/shader.cpp
  ../../src/bsdf.cpp
  ../../src/buffer.cpp
  ../../src/light.cpp
  ../../src/material.cpp
  ../../src/mesh.cpp
  ../../src/sampling.cpp
  ../../src/scene.cpp
  ../../src/accel/bvh.cpp
  ../../src/film/file.cpp
  ../../src/kernels/cpu/stream_bvh_kernel.cpp
  ../../src/kernels/cpu/linear_bvh_kernel.cpp
  ../../src/kernels/cpu/spt.hpp
  ../../src/xpu.cpp
  ../../src/xpu/cpu.cpp)

set_target_properties(_phosphoros PROPERTIES PREFIX "")

if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  set( BLENDER_LIBS ../../deps/blender/lib/darwin )
else () # i.e. Linux. Windows is not supported
  set( BLENDER_LIBS ../../deps/blender/lib/ )
endif ()

include_directories(
 ../../src
 ../../dist/include
 ../../deps/blender/git/source/blender/render/extern/include
 ../../deps/blender/git/source/blender/makesrna
 ../../deps/blender/build_darwin/source/blender/makesrna/intern
 ../../deps/blender/git/source/blender/blenlib
 ../../deps/blender/git/intern/guardedalloc
 ../../deps/blender/git/intern/mikktspace
 ../../deps/blender/git/source/blender/makesdna
 ${BLENDER_LIBS}/python/include/python3.10)

execute_process (
  COMMAND llvm-config --libdir
  OUTPUT_VARIABLE LLVM_LIBRARIES
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

target_link_directories(_phosphoros
  PUBLIC
  ../../dist/lib
  ../../deps/blender/lib/darwin/pugixml/lib
  ../../deps/blender/lib/darwin/jpeg/lib
  ../../deps/blender/lib/darwin/png/lib
  ../../deps/blender/lib/darwin/tiff/lib
  ${LLVM_LIBRARIES}
)

target_link_libraries(_phosphoros
  xml2
  pugixml
  curses
  m
  z
  oslcomp
  oslquery
  oslexec
  LLVM
  clang
  clang-cpp
  Imath
  Iex
  OpenEXR
  OpenEXRCore
  OpenEXRUtil
  OpenImageIO
  OpenImageIO_Util
  IlmThread
  pugixml
  jpeg
  png
  tiff)

target_compile_options(_phosphoros PUBLIC
  -march=native -fno-rtti)

set( CMAKE_CC_COMPILER "clang")
set( CMAKE_CXX_COMPILER "clang++")
set( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O3" )
set( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_CXX_LDFLAGS} ${LIBRARY_PATHS} ${LIBRARIES} -flto -undefined dynamic_lookup" )

set( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -g -D_DEBUG" )
