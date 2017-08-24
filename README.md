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

### Algorithms

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

Various other algorithms.

### Technical details

* Supported operating systems: Linux, Windows.
* Programming languages: C++17, GLSL 4.50.
* Supported C++ compilers: GCC 7.1, Clang 5.
* Libraries: FreeType, GLM, GMP, OpenGL 4.5, Qt 5, SFML, XLib.
* Build system: CMake.

### Screenshots

Points in 3D and the surface reconstructed from the points

![points](https://i.imgur.com/OKifv3f.png)
![reconstructed surface](https://i.imgur.com/yhoFrz4.png)
![reconstructed surface wireframe](https://i.imgur.com/wcqcTsN.png)
