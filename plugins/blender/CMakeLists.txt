cmake_minimum_required (VERSION 3.11)
project (blender)

add_library(_phosphoros SHARED
  blender.cpp
  session.cpp
  sink.cpp
  ../../src/bsdf.cpp
  ../../src/light.cpp
  ../../src/material.cpp
  ../../src/mesh.cpp
  ../../src/sampling.cpp
  ../../src/scene.cpp
  ../../src/accel/bvh.cpp
  ../../src/film/file.cpp
  ../../src/kernels/cpu/stream_bvh_kernel.cpp
  ../../src/kernels/cpu/linear_bvh_kernel.cpp
  ../../src/xpu.cpp
  ../../src/xpu/cpu.cpp)

include_directories(
 ../../src
 /usr/local/include
 /usr/local/include/OpenEXR
 /Users/jan/Code/fun/blender/source/blender/render/extern/include
 /Users/jan/Code/fun/blender/source/blender/makesrna
 /Users/jan/Code/fun/build_darwin/source/blender/makesrna/intern
 /Users/jan/Code/fun/blender/source/blender/blenlib
 /Users/jan/Code/fun/blender/intern/guardedalloc
 /Users/jan/Code/fun/blender/intern/mikktspace
 /Users/jan/Code/fun/blender/source/blender/makesdna
 /Users/jan/Code/fun/lib/darwin/python/include/python3.7m)

execute_process (COMMAND /usr/local/Cellar/llvm/7.0.1/bin/llvm-config --libfiles
                 OUTPUT_VARIABLE LLVM_LIBRARIES
                 OUTPUT_STRIP_TRAILING_WHITESPACE)

foreach (COMPONENT clangFrontend clangDriver clangSerialization
                   clangParse clangSema clangAnalysis clangAST clangBasic
                   clangEdit clangLex)
    find_library ( _CLANG_${COMPONENT}_LIBRARY
                  NAMES ${COMPONENT}
                  PATHS /usr/local/Cellar/llvm/7.0.1/lib)
    if (_CLANG_${COMPONENT}_LIBRARY)
        list (APPEND _CLANG_LIBRARIES ${_CLANG_${COMPONENT}_LIBRARY})
    endif ()
endforeach ()

list (JOIN _CLANG_LIBRARIES " " CLANG_LIBRARIES)

SET( LIBRARY_PATHS "-L/usr/local/lib -L/Users/jan/Code/fun/lib/darwin/python/lib -L/Users/jan/Code/fun/build_darwin/lib" )
SET( LIBRARIES "-lxml2 -lcurses -lm -lz -lIlmImf -lHalf -lIex -lOpenImageIO -lOpenImageIO_Util /usr/local/lib/liboslexec.a /usr/local/lib/liboslcomp.a /usr/local/lib/liboslquery.a /usr/local/lib/pugixml-1.8/libpugixml.a /usr/local/lib/libpartio.a ${LLVM_LIBRARIES} ${CLANG_LIBRARIES}" )
SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -O3 -march=native -fno-rtti " )
SET( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_CXX_LDFLAGS} ${LIBRARY_PATHS} ${LIBRARIES} -flto -undefined dynamic_lookup" )

set( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${INCLUDE_PATHS} -std=c++14 -march=native -fno-rtti -D_DEBUG" )
