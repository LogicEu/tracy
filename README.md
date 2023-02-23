# tracy

> minimal path tracer in C

![alt text](https://github.com/LogicEu/tracy/blob/main/images/traced.png?raw=true)

## About

> Tracy is a simple path tracer that runs entirely in the CPU. It uses
> a mixture between ray tracing and path tracing to render 3D scenes. Some of
> the features tracy supports are:

* Materials (Lambertian, Emissive, Dielectric, Metalic)
* Sphere, Box and Triangle Intersections
* Loading of 3D Models (.obj)
* Sparse Voxel Octree for Model Rendering Acceleration
* Multithreading
* Realtime Rendering
* Custom Scene Description File Format

> Tracy has two verions; the cli version works only from the command line and
> has no graphical user interface. Useful to perfom long and detailed renders.
> The runtime version is interactive and uses OpenGL to render the traced
> texture in real time.

## Submodules

* [utopia](https://github.com/LogicEu/utopia.git) Collection data structures in C
* [fract](https://github.com/LogicEu/fract.git) 2D and 3D math for graphics and games
* [photon](https://github.com/LogicEu/photon.git) Shapes and intersection algorithms
* [mass](https://github.com/LogicEu/mass.git) 3D model loader library
* [imgtool](https://github.com/LogicEu/imgtool.git) Save and load images easily
* [spxe](https://github.com/LogicEu/spxe.git) Simple pixel engine and renderer

## Third Party Dependencies

> The main dependencies are very common:

* [libjpeg](https://github.com/thorfdbg/libjpeg.git)
* [libpng](https://github.com/glennrp/libpng.git)
* [libz](https://github.com/madler/zlib.git)

> The runtime version of tracy depends on OpenGL libraries aswell:

* [GLFW](https://github.com/glfw/glfw.git)
* [GLEW](https://github.com/nigels-com/glew.git) (only on Linux and Windows)

## Try it

> If you have all third party dependencies installed on your system you can
> easily try tracy with the following commands:

```shell
git clone --recursive https://github.com/LogicEu/tracy.git
cd tracy
make all -j # or ./build.sh all
./tracy scenes/scene.scx
```

## Compile

> Both build script currently work on Linux an MacOS.

* Runtime

```shell
make -j # or ./build.sh rt
```

* Command Line Interface

```shell
make cli -j # or ./build.sh cli
```
