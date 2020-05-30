# Math Viewer

Just for fun with mathematics and stuff

<img src="https://i.imgur.com/HjDmj4C.png" alt="image" width="50%"></img>

## Contents

* [File types](#file-types)
* [Rendering](#rendering)
  * [GPU](#gpu)
  * [CPU](#cpu)
* [Algorithms](#algorithms)
  * [2-space](#2-space)
  * [Spaces with arbitrary number of dimensions](#spaces-with-arbitrary-number-of-dimensions)
* [Technical details](#technical-details)
* [Images](#images)
  * [4-manifold reconstruction and path tracing in 5-space](#4-manifold-reconstruction-and-path-tracing-in-5-space)
  * [3-manifold reconstruction and path tracing in 4-space](#3-manifold-reconstruction-and-path-tracing-in-4-space)
  * [2-manifold reconstruction and path tracing in 3-space](#2-manifold-reconstruction-and-path-tracing-in-3-space)
  * [2D discrete Fourier transform](#2d-discrete-fourier-transform)
  * [3D model](#3d-model)

## File types

* OBJ and STL files with objects in 3-space.
* Extended OBJ and STL files with objects in n-space.
* Text files with the coordinates of points in n-space.

## Rendering

### GPU

* Triangle meshes.
* Volumes (ray marching).
* Isosurfaces (ray marching).

### CPU

* (n-1)-simplex meshes in n-space on (n-1)-dimensional screen (path tracing).

## Algorithms

### 2-space

C++ and GLSL

Domain                                         | Algorithm
-----------------------------------------------|----------------------------------------------------------
Discrete Fourier transform for arbitrary sizes | Bluestein's algorithm and radix-2 fast Fourier transform
Optical flow                                   | Pyramidal Lucas-Kanade
Convex hull                                    | Divide et impera

### Spaces with arbitrary number of dimensions

C++

Domain                                          | Algorithm
------------------------------------------------|----------------------------------------------------
Convex hull                                     | Randomized incremental
Delaunay triangulation                          | Convex hull of paraboloid
Voronoi diagram                                 | The Delaunay triangulation
Manifold reconstruction                         | Cocone
Manifold reconstruction with boundary detection | BoundCocone
Euclidean minimum spanning tree                 | Kruskal’s algorithm and the Delaunay triangulation
Intersection of hyperplanes                     | Gaussian elimination
Intersection of convex polytopes                | The simplex algorithm
Ray intersection acceleration                   | Spatial subdivision and 2<sup>d</sup>-trees

## Technical details

Property                  | Value
--------------------------|--------------------------------------------
Programming languages     | C++20, GLSL 4.50
C++ compilers             | GCC 10, Clang 10
Platforms                 | Linux, Windows
Graphics and compute APIs | Vulkan 1.1
Frameworks                | Qt 5
Libraries                 | FreeType, GMP, Xlib
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

<img src="https://i.imgur.com/j1kUkGQ.png" alt="image" width="15%"></img>
<img src="https://i.imgur.com/A8hpwM7.png" alt="image" width="15%"></img>
<img src="https://i.imgur.com/rRXVL80.png" alt="image" width="15%"></img>
<img src="https://i.imgur.com/bimmCBL.png" alt="image" width="15%"></img>
<img src="https://i.imgur.com/7fEn0iy.png" alt="image" width="15%"></img>
<img src="https://i.imgur.com/m5FVGza.png" alt="image" width="15%"></img>

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

<img src="https://i.imgur.com/OujXP6E.png" alt="image" width="10%"></img>
<img src="https://i.imgur.com/wYGS7Ny.png" alt="image" width="10%"></img>
<img src="https://i.imgur.com/YBrlxnL.png" alt="image" width="10%"></img>
<img src="https://i.imgur.com/p4uN0Rp.png" alt="image" width="10%"></img>
<img src="https://i.imgur.com/v6m4qnj.png" alt="image" width="10%"></img>
<img src="https://i.imgur.com/fet4mhg.png" alt="image" width="10%"></img>
<img src="https://i.imgur.com/mudGwKv.png" alt="image" width="10%"></img>
<img src="https://i.imgur.com/n5reerR.png" alt="image" width="10%"></img>
<img src="https://i.imgur.com/OGVUbxe.png" alt="image" width="10%"></img>
<img src="https://i.imgur.com/aDG9Qjv.png" alt="image" width="10%"></img>
<img src="https://i.imgur.com/xC8iW26.png" alt="image" width="10%"></img>
<img src="https://i.imgur.com/T1pdgZt.png" alt="image" width="10%"></img>
<img src="https://i.imgur.com/XQ1UEz3.png" alt="image" width="10%"></img>

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

<img src="https://i.imgur.com/h3yaTRd.png" alt="image" width="18%"></img>
<img src="https://i.imgur.com/oC4wcfw.png" alt="image" width="18%"></img>
<img src="https://i.imgur.com/KfOxuOu.png" alt="image" width="18%"></img>
<img src="https://i.imgur.com/kFCwTJM.png" alt="image" width="18%"></img>
<img src="https://i.imgur.com/Vh5Lrcv.png" alt="image" width="18%"></img>

<img src="https://i.imgur.com/8oyND91.png" alt="image" width="18%"></img>

### 2D discrete Fourier transform

Real-time computing on GPU.

<img src="https://i.imgur.com/YnyQWd4.png" alt="image" width="20%"></img>

<img src="https://i.imgur.com/uvHQZmg.png" alt="image" width="20%"></img>

<img src="https://i.imgur.com/5dGkQR9.png" alt="image" width="20%"></img>

### 3D model

Real-time rendering on GPU.

<img src="https://i.imgur.com/eqxSJpD.png" alt="image" width="30%"></img>

Path tracing on CPU.

<img src="https://i.imgur.com/kumiuEr.png" alt="image" width="30%"></img>

<img src="https://i.imgur.com/y9jdovk.png" alt="image" width="30%"></img>
