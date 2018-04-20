cmake_minimum_required (VERSION 3.11)
project (phosphorus)

add_executable(phosphorus src/core.cpp src/codecs/scene.cpp)

SET( INCLUDE_PATHS "-I${PROJECT_SOURCE_DIR}/src -I/usr/local/include/ -I/usr/local/include/OpenEXR" )
SET( LIBRARY_PATHS "-L/usr/local/lib" )
SET( LIBRARIES "-lIlmImf -lHalf -lIex -lOpenImageIO -lOpenImageIO_Util -lyaml-cpp -lalembic" )
SET( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${INCLUDE_PATHS} -std=c++14 -O3" )
SET( CMAKE_EXE_LINKER_FLAGS "${CMAKE_CXX_LDFLAGS} ${LIBRARY_PATHS} ${LIBRARIES}" )

