# phosphorus
A physically based hybrid CPU/GPU renderer

The project is a personal show case project of a physically based, and performance minded renderer. The goal was to write a hybrid renderer for both GPU, and CPU use. The GPU part is currently in a rewrite to take advantage of NVIDIA's ray tracing hardware support. The CPU renderer supports most of the features Blender does. One important area that isn't properly supported is Hair, and Sub Surface scattering, which I'm currently working on.

The project currently supports scene imports via [http://www.alembic.io](Alembic), or Blender. 

## Getting Started

Top build the project, you need to have the OpenShadingLanguage, and OpenImageIO libraries installed. These should be compiled manually, since the versions in homebrew, or your package manager might not match the versions used in this project. I have not implemented any automated dpendency management at the moment. The project used OSL 1.11, and OpenImageIO 2.1.

After that, build the project with cmake.

    mkdir build
    cd build
    cmake -DCMAKE_BUILD_TYPE=Release ..
    make
    
There should be a binary in the build directory. Currently the supplied OSL shaders need to be compiled by hand. From the project root, please do:

    cd src/shaders
    for i in $(ls *.osl); do oslc -I. $i; done
    mkdir ../../build/shaders
    cp *.oso ../../build/shaders/
    
After that you should be able to use the renderer.

## Example Renders

![Blender BMW example](examples/bmw.png?raw=true "Blender BMW example")
