cmake_minimum_required (VERSION 3.11)
project (phosphorus)

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake)

add_executable(phosphorus
  src/core.cpp
  src/material.cpp
  src/mesh.cpp
  src/sampling.cpp
  src/scene.cpp
  src/accel/bvh.cpp
  src/codecs/scene.cpp
  src/kernels/cpu/stream_bvh_kernel.cpp
  src/xpu.cpp
  src/xpu/cpu.cpp)

SET( INCLUDE_PATHS "-I${PROJECT_SOURCE_DIR}/src -I/usr/local/include/ -I/usr/local/include/OpenEXR" )
SET( LIBRARY_PATHS "-L/usr/local/lib" )
SET( LIBRARIES "-lIlmImf -lHalf -lIex -lOpenImageIO -lOpenImageIO_Util -lyaml-cpp -lalembic -loslexec -loslcomp -loslquery" )
SET( CMAKE_CXX_FLAGS_RELEASE  "${CMAKE_CXX_FLAGS} ${INCLUDE_PATHS} -std=c++14 -O3 -march=native -fno-rtti" )
SET( CMAKE_EXE_LINKER_FLAGS "${CMAKE_CXX_LDFLAGS} ${LIBRARY_PATHS} ${LIBRARIES} -flto" )

set( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${INCLUDE_PATHS} -std=c++14 -march=native -fno-rtti" )

