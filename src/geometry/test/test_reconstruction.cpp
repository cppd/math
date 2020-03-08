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

#include "../cocone/reconstruction.h"
#include "../objects/points.h"

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/math.h>
#include <src/com/names.h>
#include <src/com/print.h>
#include <src/com/time.h>
#include <src/model/mesh_utility.h>
#include <src/utility/file/sys.h>
#include <src/utility/random/engine.h>

#include <cmath>
#include <random>
#include <tuple>
#include <unordered_set>

// Для BoundCocone
constexpr double RHO = 0.3;
constexpr double ALPHA = 0.14;

enum class Algorithms
{
        Cocone,
        BoundCocone
};

namespace
{
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
void test_geometry_files(
        const std::string& name,
        const std::vector<Vector<N, float>>& points,
        const std::vector<Vector<N, double>>& normals,
        const std::vector<std::array<int, N>>& facets,
        ProgressRatio* progress)
{
        static_assert(N >= 3);

        LOG("Test saving and loading");

        std::string file_name = temp_directory() + "/" + name + "." + mesh::obj_file_extension(N);

        LOG("mesh for facets...");
        std::unique_ptr<mesh::Mesh<N>> mesh1 = mesh::create_mesh_for_facets(points, normals, facets);

        LOG("save geometry...");
        std::string comment;
        comment += "Manifold Reconstruction\n";
        comment += name + "\n";
        comment += "vertices = " + to_string(mesh1->vertices.size()) + "\n";
        comment += "normals = " + to_string(mesh1->normals.size()) + "\n";
        comment += "facets = " + to_string(mesh1->facets.size());
        file_name = save_geometry(*mesh1, file_name, comment);

        LOG("load geometry...");
        std::unique_ptr<mesh::Mesh<N>> mesh2 = mesh::load_geometry<N>(file_name, progress);

        LOG("compare mesh...");
        if (mesh1->vertices.size() != mesh2->vertices.size() || mesh1->normals.size() != mesh2->normals.size() ||
            mesh1->texcoords.size() != mesh2->texcoords.size() || mesh1->facets.size() != mesh2->facets.size() ||
            mesh1->points.size() != mesh2->points.size() || mesh1->lines.size() != mesh2->lines.size() ||
            mesh1->materials.size() != mesh2->materials.size() || mesh1->images.size() != mesh2->images.size())
        {
                error("Error writing and reading geometry files");
        }
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
                        (expected_facets_min == expected_facets_max) ?
                                to_string(expected_facets_min) :
                                "[" + to_string(expected_facets_min) + ", " + to_string(expected_facets_max) + "]";

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

                std::string bound_facet_count_str = (expected_bound_facets_min == expected_bound_facets_max) ?
                                                            to_string(expected_bound_facets_min) :
                                                            "[" + to_string(expected_bound_facets_min) + ", " +
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
                create_object_repository<N>()->sphere_with_notch(point_count), progress);

        LOG("\n--- Bound " + to_string(N - 1) + "-manifold reconstructions in " + space_name(N) + " ---\n");
        all_tests<N>(
                space_name(N) + ", bounded " + to_string(N - 1) + "-manifold",
                std::unordered_set<Algorithms>{Algorithms::BoundCocone},
                create_object_repository<N>()->sphere_with_notch_bound(point_count), progress);
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
