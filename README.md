# Math Viewer

Just for fun with mathematics.

## Contents

* [Description](#description)
* [Algorithms](#algorithms)
  * [Algorithms in 2-space](#algorithms-in-2-space)
  * [Algorithms in spaces with arbitrary number of dimensions](#algorithms-in-spaces-with-arbitrary-number-of-dimensions)
* [Technical details](#technical-details)
* [Screenshots](#screenshots)
  * [4-manifold reconstruction and path tracing in 5-space](#4-manifold-reconstruction-and-path-tracing-in-5-space)
  * [Points in 3-space and the surface reconstructed from the points](#points-in-3-space-and-the-surface-reconstructed-from-the-points)

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

Property                    | Value
----------------------------|---------------------------------------------
Programming languages       | C++17, GLSL 4.50
Supported C++ compilers     | GCC 7, Clang 6
Supported operating systems | Linux, Windows
Libraries                   | FreeType, GMP, OpenGL 4.5, Qt 5, SFML, XLib
Build system                | CMake

## Screenshots

### 4-manifold reconstruction and path tracing in 5-space

2D slices of a 4D image of the 4-manifold reconstructed from the points on the 4-sphere with a hollow.

* 5-space.
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

### Points in 3-space and the surface reconstructed from the points

![points](https://i.imgur.com/TS6ct8G.png)
![reconstructed surface](https://i.imgur.com/4AU5rTu.png)
![reconstructed surface](https://i.imgur.com/wkpsz8T.png)
![reconstructed surface wireframe](https://i.imgur.com/CJ0KCcT.png)
![reconstructed surface image](https://i.imgur.com/Bczgqjw.png)
