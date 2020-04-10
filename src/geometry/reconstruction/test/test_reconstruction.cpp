/*
Copyright (C) 2017-2020 Topological Manifold

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "test_reconstruction.h"

#include "../cocone.h"

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/math.h>
#include <src/com/names.h>
#include <src/com/print.h>
#include <src/com/time.h>
#include <src/model/mesh_utility.h>
#include <src/numerical/random.h>
#include <src/utility/file/sys.h>
#include <src/utility/random/engine.h>

#include <cmath>
#include <random>
#include <tuple>
#include <unordered_set>

namespace geometry
{
namespace
{
// Для BoundCocone
constexpr double RHO = 0.3;
constexpr double ALPHA = 0.14;

constexpr double COS_FOR_BOUND = -0.3;

enum class Algorithms
{
        Cocone,
        BoundCocone
};

template <typename T, size_t... I, typename V>
constexpr Vector<sizeof...(I) + 1, T> make_last_axis(V&& value, std::integer_sequence<size_t, I...>&&)
{
        return {(static_cast<void>(I), 0)..., std::forward<V>(value)};
}

template <size_t N, typename T>
constexpr Vector<N, T> LAST_AXIS = make_last_axis<T>(1, std::make_integer_sequence<size_t, N - 1>());

template <size_t N, typename T, typename RandomEngine>
Vector<N, T> random_on_sphere(RandomEngine& engine)
{
        static_assert(std::is_floating_point_v<T>);
        std::uniform_real_distribution<T> urd(-1.0, 1.0);
        Vector<N, T> v;
        do
        {
                v = random_vector<N, T>(engine, urd);
        } while (dot(v, v) > 1);
        return v.normalized();
}

template <size_t N, typename T, typename RandomEngine>
Vector<N, T> random_on_sphere(RandomEngine& engine, bool bound)
{
        if (!bound)
        {
                return random_on_sphere<N, T>(engine);
        }
        Vector<N, T> v;
        do
        {
                v = random_on_sphere<N, T>(engine);
        } while (dot(v, LAST_AXIS<N, T>) < COS_FOR_BOUND);
        return v;
}

// Точки на сфере с углублением со стороны последней оси
// в положительном направлении этой оси
template <size_t N>
std::vector<Vector<N, float>> points_sphere_with_notch(unsigned point_count, bool bound)
{
        std::mt19937_64 engine(point_count);

        std::vector<Vector<N, float>> points;

        while (points.size() < point_count)
        {
                Vector<N, double> v = random_on_sphere<N, double>(engine, bound);

                double dot_z = dot(LAST_AXIS<N, double>, v);
                if (dot_z > 0)
                {
                        v[N - 1] *= 1 - std::abs(0.5 * std::pow(dot_z, 5));
                }

                points.push_back(to_vector<float>(v));
        }

        return points;
}

template <size_t N>
constexpr std::tuple<unsigned, unsigned> facet_count(unsigned point_count)
{
        static_assert(2 <= N && N <= 4);

        if constexpr (N == 2)
        {
                ASSERT(point_count >= 3);
                return {point_count, point_count};
        }

        if constexpr (N == 3)
        {
                ASSERT(point_count >= 4);
                // Mark de Berg, Otfried Cheong, Marc van Kreveld, Mark Overmars
                // Computational Geometry. Algorithms and Applications. Third Edition.
                // Theorem 11.1.
                unsigned count = 2 * point_count - 4;
                return {count, count};
        }

        if constexpr (N == 4)
        {
                ASSERT(point_count >= 5);
                // Handbook of Discrete and Computational Geometry edited by Jacob E. Goodman and Joseph O’Rourke.
                // Second edition.
                // 22.3 COMPUTING COMBINATORIAL DESCRIPTIONS.
                // Точно не определить, так как зависит от триангуляции.
                // Опыты с выпуклой оболочкой со случайным распределением точек на четырёхмерной
                // сфере дают отношение количества граней к количеству точек около 6.7.
                unsigned min = std::lround(6.55 * point_count);
                unsigned max = std::lround(6.85 * point_count);
                return {min, max};
        }
}

template <size_t N>
void test_obj_file(
        const mesh::Mesh<N>& mesh,
        const std::string& file_name,
        const std::string& comment,
        ProgressRatio* progress)
{
        LOG("saving to OBJ...");

        std::string saved_file = mesh::save_to_obj(mesh, file_name + "." + mesh::obj_file_extension(N), comment);

        LOG("loading from OBJ...");

        std::unique_ptr<const mesh::Mesh<N>> file_mesh = mesh::load<N>(saved_file, progress);

        LOG("comparing meshes...");

        if (mesh.vertices.size() != file_mesh->vertices.size())
        {
                error("Error writing and reading OBJ files (vertices)");
        }
        if (mesh.normals.size() != file_mesh->normals.size())
        {
                error("Error writing and reading OBJ files (normals)");
        }
        if (mesh.texcoords.size() != file_mesh->texcoords.size())
        {
                error("Error writing and reading OBJ files (texture)");
        }
        if (mesh.facets.size() != file_mesh->facets.size())
        {
                error("Error writing and reading OBJ files (facets)");
        }
        if (mesh.points.size() != file_mesh->points.size())
        {
                error("Error writing and reading OBJ files (points)");
        }
        if (mesh.lines.size() != file_mesh->lines.size())
        {
                error("Error writing and reading OBJ files (lines)");
        }
        if (mesh.materials.size() != file_mesh->materials.size())
        {
                error("Error writing and reading OBJ files (materials)");
        }
        if (mesh.images.size() != file_mesh->images.size())
        {
                error("Error writing and reading OBJ files (images)");
        }
}

template <size_t N>
void test_stl_file(
        const mesh::Mesh<N>& mesh,
        const std::string& file_name,
        const std::string& comment,
        ProgressRatio* progress,
        bool ascii_format)
{
        const std::string type_name = ascii_format ? "ASCII" : "binary";

        LOG("saving to " + type_name + " STL...");

        std::string saved_file =
                mesh::save_to_stl(mesh, file_name + "." + mesh::stl_file_extension(N), comment, ascii_format);

        LOG("loading from " + type_name + " STL...");

        std::unique_ptr<const mesh::Mesh<N>> file_mesh = mesh::load<N>(saved_file, progress);

        LOG("comparing meshes...");

        if (mesh.vertices.size() != file_mesh->vertices.size())
        {
                error("Error writing and reading STL files (vertices)");
        }
        if (mesh.facets.size() != file_mesh->facets.size())
        {
                error("Error writing and reading STL files (facets)");
        }
}

template <size_t N>
void test_geometry_files(
        const std::string& name,
        const std::vector<Vector<N, float>>& points,
        const std::vector<Vector<N, double>>& normals,
        const std::vector<std::array<int, N>>& facets,
        ProgressRatio* progress)
{
        static_assert(N >= 3);

        LOG("Test saving and loading");

        std::string file_name = temp_directory() + "/" + name;

        LOG("creating mesh for facets...");
        std::unique_ptr<const mesh::Mesh<N>> mesh = mesh::create_mesh_for_facets(points, normals, facets);

        std::string comment;
        comment += "Manifold Reconstruction\n";
        comment += name + "\n";
        comment += "vertices = " + to_string(mesh->vertices.size()) + "\n";
        comment += "normals = " + to_string(mesh->normals.size()) + "\n";
        comment += "facets = " + to_string(mesh->facets.size());

        test_obj_file(*mesh, file_name, comment, progress);

        test_stl_file(*mesh, file_name, comment, progress, true /*ascii_format*/);
        test_stl_file(*mesh, file_name, comment, progress, false /*ascii_format*/);
}

// Файлы не поддерживают двухмерные объекты
void test_geometry_files(
        const std::string&,
        const std::vector<Vector<2, float>>&,
        const std::vector<Vector<2, double>>&,
        const std::vector<std::array<int, 2>>&,
        ProgressRatio*)
{
}

template <size_t N>
std::vector<Vector<N, float>> clone_object(
        const std::vector<Vector<N, float>>& points,
        unsigned new_object_count,
        float shift)
{
        ASSERT(new_object_count > 1 && new_object_count <= (1 << N));

        unsigned all_object_count = (1 + new_object_count);

        std::vector<Vector<N, float>> clones(points.begin(), points.end());

        clones.reserve(points.size() * all_object_count);

        for (unsigned new_object = 0; new_object < new_object_count; ++new_object)
        {
                Vector<N, float> vec_shift;
                for (unsigned n = 0; n < N; ++n)
                {
                        vec_shift[n] = ((1 << n) & new_object) ? shift : -shift;
                }
                for (unsigned i = 0; i < points.size(); ++i)
                {
                        clones.push_back(points[i] + vec_shift);
                }
        }

        ASSERT(clones.size() == points.size() * all_object_count);

        return clones;
}

template <size_t N>
void test_algorithms(
        const std::string& name,
        const std::unordered_set<Algorithms>& algorithms,
        double rho,
        double alpha,
        const std::vector<Vector<N, float>>& points,
        unsigned expected_facets_min,
        unsigned expected_facets_max,
        unsigned expected_bound_facets_min,
        unsigned expected_bound_facets_max,
        ProgressRatio* progress)
{
        ASSERT(points.size() > N);
        ASSERT(expected_facets_min > 0 && expected_facets_max > 0 && expected_bound_facets_min > 0 &&
               expected_bound_facets_max > 0);

        double start_time = time_in_seconds();

        LOG("Point count: " + to_string(points.size()));

        std::unique_ptr<ManifoldConstructor<N>> sr = create_manifold_constructor(points, progress);

        if (algorithms.count(Algorithms::Cocone))
        {
                // Задать размер для проверки очистки массивов
                std::vector<Vector<N, double>> normals(10000);
                std::vector<std::array<int, N>> facets(10000);

                std::string facet_count_str =
                        (expected_facets_min == expected_facets_max)
                                ? to_string(expected_facets_min)
                                : "[" + to_string(expected_facets_min) + ", " + to_string(expected_facets_max) + "]";

                LOG("Expected facet count: " + facet_count_str);

                sr->cocone(&normals, &facets, progress);

                LOG("Cocone facet count: " + to_string(facets.size()));
                if (!(expected_facets_min <= facets.size() && facets.size() <= expected_facets_max))
                {
                        error("Error facet count: expected " + facet_count_str + ", Cocone computed " +
                              to_string(facets.size()));
                }

                test_geometry_files(name + ", Cocone", points, normals, facets, progress);
        }

        if (algorithms.count(Algorithms::BoundCocone))
        {
                // Задать размер для проверки очистки массивов
                std::vector<Vector<N, double>> normals(10000);
                std::vector<std::array<int, N>> facets(10000);

                std::string bound_facet_count_str = (expected_bound_facets_min == expected_bound_facets_max)
                                                            ? to_string(expected_bound_facets_min)
                                                            : "[" + to_string(expected_bound_facets_min) + ", " +
                                                                      to_string(expected_bound_facets_max) + "]";

                LOG("Expected bound facet count: " + bound_facet_count_str);

                sr->bound_cocone(rho, alpha, &normals, &facets, progress);

                LOG("BoundCocone facet count: " + to_string(facets.size()));
                if (!(expected_bound_facets_min <= facets.size() && facets.size() <= expected_bound_facets_max))
                {
                        error("Error bound facet count: expected " + bound_facet_count_str + ", BoundCocone computed " +
                              to_string(facets.size()));
                }

                test_geometry_files(name + ", BoundCocone", points, normals, facets, progress);
        }

        LOG("Time: " + to_string_fixed(time_in_seconds() - start_time, 5) + " s");
        LOG("Successful manifold reconstruction in " + space_name(N));
}

template <size_t N>
void all_tests(
        const std::string& name,
        const std::unordered_set<Algorithms>& algorithms,
        std::vector<Vector<N, float>>&& points,
        ProgressRatio* progress)
{
        static_assert(2 <= N && N <= 4);

        // BoundCocone может давать разные результаты в зависимости от точек и параметров,
        // поэтому надо проверять попадание в интервал, а не равенство
        constexpr double bound_low_coef = 0.9;
        constexpr double bound_high_coef = 1.1;

        // Объект находится в начале координат и имеет размеры не более 1 по каждой координате
        // в обе стороны, поэтому достаточно смещение на 3 для отсутствия пересечений объектов
        constexpr float shift = 3;

        auto [facets_min, facets_max] = facet_count<N>(points.size());
        unsigned bound_facets_min = std::lround(bound_low_coef * facets_min);
        unsigned bound_facets_max = std::lround(bound_high_coef * facets_max);

        LOG("------- " + space_name(N) + ", 1 object -------");

        test_algorithms(
                name + ", 1 object", algorithms, RHO, ALPHA, points, facets_min, facets_max, bound_facets_min,
                bound_facets_max, progress);

        LOG("");

        // Разместить вокруг объекта другие такие же объекты по каждой координате в обе стороны

        constexpr unsigned new_object_count = 1 << N;
        constexpr unsigned all_object_count = (1 + new_object_count);

        LOG("------- " + space_name(N) + ", " + to_string(all_object_count) + " objects -------");

        points = clone_object(points, new_object_count, shift);

        facets_min *= all_object_count;
        facets_max *= all_object_count;
        bound_facets_min *= all_object_count;
        bound_facets_max *= all_object_count;

        test_algorithms(
                name + ", " + to_string(all_object_count) + " objects", algorithms, RHO, ALPHA, points, facets_min,
                facets_max, bound_facets_min, bound_facets_max, progress);
}

template <size_t N>
void test(int low, int high, ProgressRatio* progress)
{
        thread_local RandomEngineWithSeed<std::mt19937_64> engine;

        int point_count = std::uniform_int_distribution<int>(low, high)(engine);

        LOG("\n--- Unbound " + to_string(N - 1) + "-manifold reconstructions in " + space_name(N) + " ---\n");
        all_tests<N>(
                space_name(N) + ", unbounded " + to_string(N - 1) + "-manifold",
                std::unordered_set<Algorithms>{Algorithms::Cocone, Algorithms::BoundCocone},
                points_sphere_with_notch<N>(point_count, false), progress);

        LOG("\n--- Bound " + to_string(N - 1) + "-manifold reconstructions in " + space_name(N) + " ---\n");
        all_tests<N>(
                space_name(N) + ", bounded " + to_string(N - 1) + "-manifold",
                std::unordered_set<Algorithms>{Algorithms::BoundCocone}, points_sphere_with_notch<N>(point_count, true),
                progress);
}
}

void test_reconstruction(int number_of_dimensions, ProgressRatio* progress)
{
        ASSERT(progress);

        switch (number_of_dimensions)
        {
        case 2:
                test<2>(100, 1000, progress);
                break;
        case 3:
                test<3>(2000, 3000, progress);
                break;
        case 4:
                test<4>(20000, 25000, progress);
                break;
        default:
                error("Error manifold reconstruction test number of dimensions " + to_string(number_of_dimensions));
        }
}
}
