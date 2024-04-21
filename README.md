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
  * [3-manifold reconstruction and path tracing in 4-space](#3-manifold-reconstruction-and-path-tracing-in-4-space)
  * [4D Cornell box](#4d-cornell-box)
  * [Real-time rendering](#real-time-rendering)
  * [3D models](#3d-models)
  * [4D polytope](#4d-polytope)

## File types

* OBJ and STL files with objects in 3-space
* Extended OBJ and STL files with objects in n-space
* Text files with the coordinates of points in n-space

## Rendering

### GPU

* Triangle meshes (rasterization)
* Volumes (ray marching)
* Isosurfaces (ray marching)
* Shadows (ray tracing, ray marching, shadow mapping)
* Order-independent transparency (lists of fragments)

### CPU

* Bidirectional path tracing with multiple importance sampling
* (n-1)-simplex meshes in n-space on (n-1)-dimensional screen
* n-dimensional microfacet BRDF
* Spectral rendering

## Algorithms

### 2-space

C++ and GLSL

Subject                                        | Algorithms
-----------------------------------------------|---------------------------------------------------------
Discrete Fourier transform for arbitrary sizes | Bluestein's algorithm and radix-2 fast Fourier transform
Optical flow                                   | Pyramidal Lucas-Kanade
Convex hull                                    | Divide et impera

### Spaces with arbitrary number of dimensions

C++

Subject                                                 | Algorithms
--------------------------------------------------------|-----------------------------------------------------------
Convex hull                                             | Randomized incremental
Delaunay triangulation                                  | Convex hull of paraboloid
Voronoi diagram                                         | The Delaunay triangulation
Manifold reconstruction                                 | Cocone
Manifold reconstruction with boundary detection         | BoundCocone
Euclidean minimum spanning tree                         | Kruskal’s algorithm and the Delaunay triangulation
Intersection of hyperplanes                             | Gaussian elimination
Intersection of convex polytopes                        | The simplex algorithm
Ray intersection acceleration using spatial subdivision | 2<sup>d</sup>-trees
Ray intersection acceleration using object subdivision  | Bounding volume hierarchies and the surface area heuristic
Eigenvalues and eigenvectors of symmetric matrices      | Jacobi method
Least squares fitting a plane to points                 | Principal component analysis

## Technical details

Property                  | Value
--------------------------|--------------------------------------------
Programming languages     | C++23, GLSL 4.60
C++ compilers             | GCC 13, Clang 18
Platforms                 | Linux
Graphics and compute APIs | Vulkan 1.3 with ray tracing extensions
Frameworks                | Qt 6
Libraries                 | FreeType, GMP
Optional libraries        | cuFFT (for DFT tests), FFTW (for DFT tests)
Build systems             | CMake

## Images

### 3-manifold reconstruction and path tracing in 4-space

2D images of a 3D image of the 3-manifold reconstructed from the points 
on the 3-torus T<sup>3</sup> (S<sup>1</sup>×S<sup>1</sup>×S<sup>1</sup>)

* 5-space
  * The convex hull
  * 4-simplex facets
* 4-space
  * 3-simplex mesh without boundary
  * 4-parallelotope hierarchy
  * 4-dimensional microfacet BRDF
  * 3-ball light source
  * 4-dimensional soft shadows
  * Parallel projection onto 3D screen

Volumetric rendering

<img src="https://i.imgur.com/2sEU0r9.png" alt="image" width="19%"></img>
<img src="https://i.imgur.com/iQY6ACw.png" alt="image" width="19%"></img>
<img src="https://i.imgur.com/w95fl6j.png" alt="image" width="19%"></img>
<img src="https://i.imgur.com/4PpADsU.png" alt="image" width="19%"></img>
<img src="https://i.imgur.com/E3nUHLm.png" alt="image" width="19%"></img>
<img src="https://i.imgur.com/ingQCst.png" alt="image" width="19%"></img>
<img src="https://i.imgur.com/l9GsQ0K.png" alt="image" width="19%"></img>
<img src="https://i.imgur.com/QbAgg4D.png" alt="image" width="19%"></img>
<img src="https://i.imgur.com/t6Hb7xC.png" alt="image" width="19%"></img>
<img src="https://i.imgur.com/v4YNeDa.png" alt="image" width="19%"></img>
<img src="https://i.imgur.com/ZVVmvXR.png" alt="image" width="19%"></img>
<img src="https://i.imgur.com/EoJeL0V.png" alt="image" width="19%"></img>
<img src="https://i.imgur.com/XL8NzRJ.png" alt="image" width="19%"></img>
<img src="https://i.imgur.com/7rgjaqb.png" alt="image" width="19%"></img>
<img src="https://i.imgur.com/FQH6ULO.png" alt="image" width="19%"></img>

### 4D Cornell box

2D images of a 3D image of the 4-dimensional Cornell box

* Seven 3-parallelotopes (the sides of the box)
* One 3-parallelotope light source
* One 3-simplex mesh without boundary (3-sphere)
* One 4-parallelotope
* 4-dimensional microfacet BRDF
* 4-dimensional soft shadows
* Perspective projection onto 3D screen

<img src="https://i.imgur.com/oDufPGh.png" alt="image" width="23%"></img>
<img src="https://i.imgur.com/Qts0cgW.png" alt="image" width="23%"></img>
<img src="https://i.imgur.com/ojdEkWt.png" alt="image" width="23%"></img>
<img src="https://i.imgur.com/M7GMG8P.png" alt="image" width="23%"></img>

### Real-time rendering

Semitransparent objects

* Triangle mesh (green) and its ray tracing shadows
* Isosurface (blue) and its ray marching shadows

<img src="https://i.imgur.com/82dhRk9.png" alt="image" width="23%"></img>
<img src="https://i.imgur.com/gvchVTs.png" alt="image" width="23%"></img>
<img src="https://i.imgur.com/UC7VQjT.png" alt="image" width="23%"></img>
<img src="https://i.imgur.com/ifhRGra.png" alt="image" width="23%"></img>

### 3D models

Spectral rendering

1. Blackbody 2500 K
1. Daylight 4000 K
1. Daylight D65
1. Blackbody 12000 K

<img src="https://i.imgur.com/6XJs94H.png" alt="image" width="23%"></img>
<img src="https://i.imgur.com/55ZX2bE.png" alt="image" width="23%"></img>
<img src="https://i.imgur.com/VBa1A6A.png" alt="image" width="23%"></img>
<img src="https://i.imgur.com/S9YZB2N.png" alt="image" width="23%"></img>

<img src="https://i.imgur.com/b0jPI0I.png" alt="image" width="23%"></img>
<img src="https://i.imgur.com/GSz4tk8.png" alt="image" width="23%"></img>
<img src="https://i.imgur.com/w1kS1Fv.png" alt="image" width="23%"></img>
<img src="https://i.imgur.com/XsUdYnt.png" alt="image" width="23%"></img>

### 4D polytope

4-dimensional cross-polytope

```
v -11   0   0   0
v   0 -12   0   0
v   0   0 -13   0
v   0   0   0 -14
v   0   0   0  15
v   0   0  16   0
v   0  17   0   0
v  18   0   0   0
f 1 2 3 4
f 1 2 3 5
f 1 2 6 4
f 1 2 6 5
f 1 7 3 4
f 1 7 3 5
f 1 7 6 4
f 1 7 6 5
f 8 2 3 4
f 8 2 3 5
f 8 2 6 4
f 8 2 6 5
f 8 7 3 4
f 8 7 3 5
f 8 7 6 4
f 8 7 6 5
```

<img src="https://i.imgur.com/s0SHinD.png" alt="image" width="23%"></img>
<img src="https://i.imgur.com/lkC8Bgs.png" alt="image" width="23%"></img>
<img src="https://i.imgur.com/X2D1jMr.png" alt="image" width="23%"></img>
<img src="https://i.imgur.com/8MJ5Ptm.png" alt="image" width="23%"></img>
