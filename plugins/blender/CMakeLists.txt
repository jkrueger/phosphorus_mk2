cmake_minimum_required (VERSION 3.11)
project (blender)

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

include_directories(
 ../../src
 lib/include
 ${BLENDER_PATH}/git/source/blender/render/extern/include
 ${BLENDER_PATH}/git/source/blender/makesrna
 ${BLENDER_PATH}/build/source/blender/makesrna/intern
 ${BLENDER_PATH}/git/source/blender/blenlib
 ${BLENDER_PATH}/git/intern/guardedalloc
 ${BLENDER_PATH}/git/intern/mikktspace
 ${BLENDER_PATH}/git/source/blender/makesdna
 /opt/lib/python-3.10/include/python3.10)

target_link_directories(_phosphoros
  PUBLIC
  lib/lib)

target_link_libraries(_phosphoros
  xml2
  pugixml
  curses
  m
  z
  oslcomp
  oslquery
  oslexec
  LLVM-10
  clang-10
  clang-cpp-10
  Imath-3_2
  Iex-3_2
  OpenEXR-3_2
  OpenEXRCore-3_2
  OpenEXRUtil-3_2
  OpenImageIO
  OpenImageIO_Util
  IlmThread-3_2
  pugixml
  jpeg
  png
  tiff)

execute_process (COMMAND ${LLVM_PATH}/bin/llvm-config --libfiles
                 OUTPUT_VARIABLE LLVM_LIBRARIES
                 OUTPUT_STRIP_TRAILING_WHITESPACE)

foreach (COMPONENT clangFrontend clangDriver clangSerialization
                   clangParse clangSema clangAnalysis clangAST clangBasic
                   clangEdit clangLex)
    find_library ( _CLANG_${COMPONENT}_LIBRARY
                  NAMES ${COMPONENT}
                  PATHS ${LLVM_PATH}/lib)
    if (_CLANG_${COMPONENT}_LIBRARY)
        list (APPEND _CLANG_LIBRARIES ${_CLANG_${COMPONENT}_LIBRARY})
    endif ()
endforeach ()

list (JOIN _CLANG_LIBRARIES " " CLANG_LIBRARIES)

SET( CMAKE_CC_COMPILER "clang")
SET( CMAKE_CXX_COMPILER "clang++")
SET( LIBRARIES "${LLVM_LIBRARIES} ${CLANG_LIBRARIES}" )
SET( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -std=c++17 -O3 -march=native -fno-rtti" )
SET( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_CXX_LDFLAGS} ${LIBRARY_PATHS} ${LIBRARIES} -flto -undefined dynamic_lookup" )

SET( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -g -std=c++17 -march=native -fno-rtti -D_DEBUG" )
