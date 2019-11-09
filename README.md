# Math Viewer

Just for fun with mathematics and stuff

![image](https://i.imgur.com/HjDmj4C.png)

## Contents

* [Description](#description)
* [Algorithms](#algorithms)
  * [Algorithms in 2-space](#algorithms-in-2-space)
  * [Algorithms in spaces with arbitrary number of dimensions](#algorithms-in-spaces-with-arbitrary-number-of-dimensions)
* [Technical details](#technical-details)
* [Images](#images)
  * [4-manifold reconstruction and path tracing in 5-space](#4-manifold-reconstruction-and-path-tracing-in-5-space)
  * [3-manifold reconstruction and path tracing in 4-space](#3-manifold-reconstruction-and-path-tracing-in-4-space)
  * [2-manifold reconstruction and path tracing in 3-space](#2-manifold-reconstruction-and-path-tracing-in-3-space)
  * [2D discrete Fourier transform](#2d-discrete-fourier-transform)
  * [3D model](#3d-model)

## Description

External Input

* OBJ files (.obj) with vertices, normals, triangles and textures.
* Extended OBJ files (.objn) with vertices, normals and (n-1)-simplices in n-space.
* Text files with the coordinates of points in n-space, one point per line.

Internal Input

* Built-in objects.
* Rendered 2D images.

Algorithms

* A variety of mathematical algorithms (section [“Algorithms”](#algorithms)).

Output

* 2D images with the output of
  * real-time rendering of 3-space objects,
  * non-real-time rendering of n-space objects on the internal (n-1)-dimensional screen,
  * algorithms in 2-space.
* OBJ files with the output of algorithms in 3-space.
* Extended OBJ files with the output of algorithms in n-space.

## Algorithms

These are some of the implemented algorithms. There are various other algorithms.

### Algorithms in 2-space

Subject                                         | Algorithm                                                | Implementation | Language
------------------------------------------------|----------------------------------------------------------|----------------|---------
Discrete Fourier transform for arbitrary sizes  | Bluestein's algorithm and radix-2 fast Fourier transform | Parallel       | GLSL
Optical flow                                    | Pyramidal Lucas-Kanade                                   | Parallel       | GLSL
Convex hull                                     | Divide et impera                                         | Parallel       | GLSL

### Algorithms in spaces with arbitrary number of dimensions

Subject                                         | Algorithm                                          | Implementation     | Language
------------------------------------------------|----------------------------------------------------|--------------------|---------
Convex hull                                     | Randomized incremental                             | Partially parallel | C++
Delaunay triangulation                          | Convex hull of paraboloid                          | Sequential         | C++
Voronoi diagram                                 | The Delaunay triangulation                         | Sequential         | C++
Manifold reconstruction                         | Cocone                                             | Sequential         | C++
Manifold reconstruction with boundary detection | BoundCocone                                        | Sequential         | C++
Euclidean minimum spanning tree                 | Kruskal’s algorithm and the Delaunay triangulation | Sequential         | C++
Intersection of hyperplanes                     | Gaussian elimination                               | Sequential         | C++
Intersection of convex polytopes                | The simplex algorithm                              | Sequential         | C++
Ray intersection acceleration                   | Spatial subdivision and 2<sup>d</sup>-trees        | Parallel           | C++
Realistic visualization                         | Path tracing                                       | Parallel           | C++

## Technical details

Property                  | Value
--------------------------|--------------------------------------------
Programming languages     | C++20, GLSL 4.50
C++ compilers             | GCC 9, Clang 9
Platforms                 | Linux, Windows
Graphics and compute APIs | Vulkan 1.1, OpenGL 4.5
Frameworks                | Qt 5
Libraries                 | FreeType, GLFW, GMP, SFML, Xlib
Optional libraries        | cuFFT (for DFT tests), FFTW (for DFT tests)
Build systems             | CMake

## Images

### 4-manifold reconstruction and path tracing in 5-space

2D slices of a 4D image of the 4-manifold reconstructed from the points on the 4-sphere with a hollow.

* 6-space
  * The convex hull.
  * 5-simplex facets.
* 5-space
  * 4-simplex mesh without boundary.
  * 32-tree.
  * 5-parallelotopes.
  * Point light source.
  * Parallel projection onto 4D screen.

![manifold](https://i.imgur.com/j1kUkGQ.png)
![manifold](https://i.imgur.com/A8hpwM7.png)
![manifold](https://i.imgur.com/rRXVL80.png)
![manifold](https://i.imgur.com/bimmCBL.png)
![manifold](https://i.imgur.com/7fEn0iy.png)
![manifold](https://i.imgur.com/m5FVGza.png)

### 3-manifold reconstruction and path tracing in 4-space

2D slices of a 3D image of the 3-manifold reconstructed from the points on the 3-torus T<sup>3</sup> (S<sup>1</sup>×S<sup>1</sup>×S<sup>1</sup>).

* 5-space
  * The convex hull.
  * 4-simplex facets.
* 4-space
  * 3-simplex mesh without boundary.
  * 16-tree.
  * 4-parallelotopes.
  * Point light source.
  * Parallel projection onto 3D screen.

![manifold](https://i.imgur.com/OujXP6E.png)
![manifold](https://i.imgur.com/wYGS7Ny.png)
![manifold](https://i.imgur.com/YBrlxnL.png)
![manifold](https://i.imgur.com/p4uN0Rp.png)
![manifold](https://i.imgur.com/v6m4qnj.png)
![manifold](https://i.imgur.com/fet4mhg.png)
![manifold](https://i.imgur.com/mudGwKv.png)
![manifold](https://i.imgur.com/n5reerR.png)
![manifold](https://i.imgur.com/OGVUbxe.png)
![manifold](https://i.imgur.com/aDG9Qjv.png)
![manifold](https://i.imgur.com/xC8iW26.png)
![manifold](https://i.imgur.com/T1pdgZt.png)
![manifold](https://i.imgur.com/XQ1UEz3.png)

### 2-manifold reconstruction and path tracing in 3-space

Points on the (2,3)-torus knot, the Euclidean minimum spanning tree of the points, and the 2-manifold reconstructed from the points.

* 4-space
  * The convex hull.
  * 3-simplex facets.
* 3-space
  * 2-simplex mesh without boundary.
  * 8-tree.
  * 3-parallelotopes.
  * Point light source.
  * Parallel projection onto 2D screen.

![points](https://i.imgur.com/h3yaTRd.png)
![tree](https://i.imgur.com/oC4wcfw.png)
![manifold](https://i.imgur.com/KfOxuOu.png)
![manifold](https://i.imgur.com/kFCwTJM.png)
![manifold](https://i.imgur.com/Vh5Lrcv.png)
![manifold](https://i.imgur.com/8oyND91.png)

### 2D discrete Fourier transform

Real-time computing on GPU.

![dft](https://i.imgur.com/YnyQWd4.png)
![dft](https://i.imgur.com/uvHQZmg.png)
![dft](https://i.imgur.com/5dGkQR9.png)

### 3D model

Real-time rendering on GPU.

![image](https://i.imgur.com/eqxSJpD.png)

Path tracing on CPU.

![image](https://i.imgur.com/kumiuEr.png)
![image](https://i.imgur.com/y9jdovk.png)
