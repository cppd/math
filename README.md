### OBJ Math Viewer

Эта программа написана с развлекательными целями. Чтение файлов OBJ и отображение их объектов на экране.
Подача этих данных на вход разных интересных для меня алгоритмов.

Алгоритмы:

1. Чтение файлов OBJ, содержащих только вершины, нормали, треугольники и текстуры. Параллельно на центральном процессоре, std::thread.

1. Отображение прочитанных файлов OBJ на экране с тенью, каркасной моделью, вращением, масштабированием и т.д. Vertex, geometry, fragment shaders.

1. Двумерное дискретное преобразование Фурье. Для произвольного размера на основе БПФ и алгоритма Блуштейна. Параллельно на видеокарте, compute shaders.

1. Построение выпуклой оболочки в пространстве 2D по алгоритму divide et impera. Параллельно на видеокарте, compute shaders.

1. Построение выпуклой оболочки в пространствах произвольной размерности по инкрементальному алгоритму со списками конфликтов.
Для координат точек используются целые числа. Частично параллельно на центральном процессоре, std::thread.

1. Восстановление поверхностей по алгоритмам COCONE и BOUND COCONE. Программно сделано для любой размерности, но проверялось только для 2D и 3D,
так как сложно представить многообразия произвольной размерности. Последовательно на центральном процессоре.

1. Вспомогательные задачи: диаграммы Вороного и Делоне, системы уравнений по методу Гаусса и другие.


Некоторые технические подробности:

1. Работает только на Линуксах из-за небольшого использования XLib.

1. Языки программирования: C++17, GLSL 4.50.

1. Библиотеки: FreeType, GLM, GMP, OpenGL 4.5, Qt 5, SFML, XLib.

1. Система сборки: CMake.

1. Широкое применение многопоточности (std::thread).

---

This program is written just for fun. It reads OBJ files, shows their objects on the screen, and uses
them as the input data for some algorithms that are interesting to me.

Algorithms:

1. Reading OBJ files that contain only vertices, normals, triangles and textures. In parallel on the CPU, std::thread.

1. Showing the loaded OBJ file on the screen with shadows, wireframes, rotation, scaling, et cetera. Vertex, geometry, fragment shaders.

1. Two-dimensional discrete Fourier transform for arbitrary sizes. Bluestein's algorithm and FFT. In parallel on the GPU, compute shaders.

1. Computing 2D convex hulls via the divide et impera algorithm. In parallel on the GPU, compute shaders.

1. Computing convex hulls for an arbitrary number of dimensions via the randomized incremental algorithm. Implemented using integer
coordinates for the points. Partially in parallel on the CPU, std::thread.

1. Surface reconstruction via COCONE and BOUND COCONE algorithms. Although the program is implemented for an arbitrary number of dimensions,
it has been tested only for 2D and 3D because it's hard to imagine n-manifolds. Sequentially on the CPU.

1. Smaller algorithms: Voronoi diagrams, Delaunay triangulation-tetrahedralization-and-so-on, the Gauss elimination method, and others.


Some technical details:

1. Only for Linux because of a small use of XLib.

1. Programming languages: C++17, GLSL 4.50.

1. Libraries: FreeType, GLM, GMP, OpenGL 4.5, Qt 5, SFML, XLib.

1. Build system: CMake.

1. Extensive use of multithreading (std::thread).

---

### Screenshot

![screenshot](screenshots/screenshot.png?raw=true)

