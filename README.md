# Math Viewer

Just for fun with mathematics.

This program
* Reads OBJ files that contain vertices, normals, triangles and textures.
  Reads text files that contain x, y and z coordinates of points, one point per line.
* Shows 3D models on the screen with shadows, wireframes, rotation, scaling, et cetera.
* Uses points, 3D models and their 2D images as the input data for a variety of mathematical
  algorithms (see details below).
* Writes reconstructed surfaces to OBJ files.
* Uses multithreading extensively.

## Algorithms

Subject                                        | Algorithm                                                | Space                          | Implementation     | Language
-----------------------------------------------|----------------------------------------------------------|--------------------------------|--------------------|---------
Discrete Fourier transform for arbitrary sizes | Bluestein's algorithm and radix-2 fast Fourier transform | 2 dimensions                   | Parallel           | GLSL
Optical flow                                   | Pyramidal Lucas-Kanade                                   | 2 dimensions                   | Parallel           | GLSL
Convex hull                                    | Divide et impera                                         | 2 dimensions                   | Parallel           | GLSL
Convex hull                                    | Randomized incremental                                   | Arbitrary number of dimensions | Partially parallel | C++
Delaunay triangulation                         | Convex hull of paraboloid                                | Arbitrary number of dimensions | Sequential         | C++
Voronoi diagram                                | Convex hull of paraboloid                                | Arbitrary number of dimensions | Sequential         | C++
Manifold reconstruction                        | Cocone                                                   | Arbitrary number of dimensions | Sequential         | C++
Manifold reconstruction                        | BoundCocone                                              | Arbitrary number of dimensions | Sequential         | C++
Ray intersection acceleration                  | Spatial subdivision and 2<sup>d</sup>-trees              | Arbitrary number of dimensions | Parallel           | C++
Realistic visualization                        | Path tracing                                             | Arbitrary number of dimensions | Parallel           | C++

Various other algorithms.

## Technical details

* Supported operating systems: Linux, Windows.
* Programming languages: C++17, GLSL 4.50.
* Supported C++ compilers: GCC 7, Clang 5.
* Libraries: FreeType, GMP, OpenGL 4.5, Qt 5, SFML, XLib.
* Build system: CMake.

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

### Points in 3D and the surface reconstructed from the points

![points](https://i.imgur.com/TS6ct8G.png)
![reconstructed surface](https://i.imgur.com/4AU5rTu.png)
![reconstructed surface](https://i.imgur.com/wkpsz8T.png)
![reconstructed surface wireframe](https://i.imgur.com/CJ0KCcT.png)
![reconstructed surface image](https://i.imgur.com/Bczgqjw.png)
