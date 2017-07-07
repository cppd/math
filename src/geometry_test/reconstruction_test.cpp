/*
Copyright (C) 2017 Topological Manifold

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

#include "reconstruction_test.h"

#include "com/log.h"
#include "com/math.h"
#include "com/print.h"
#include "com/random.h"
#include "com/time.h"
#include "geometry_cocone/reconstruction.h"
#include "geometry_objects/points.h"
#include "progress/progress.h"

#include <cmath>
#include <random>
#include <tuple>

// Для BOUND COCONE
constexpr double RHO = 0.3;
constexpr double ALPHA = 0.14;

namespace
{
template <size_t N>
constexpr std::tuple<size_t, size_t> facet_count(size_t point_count)
{
        static_assert(2 <= N && N <= 4);

        switch (N)
        {
        case 2:
        {
                size_t cnt = point_count;
                return std::tuple<size_t, size_t>(cnt, cnt);
        }
        case 3:
        {
                // Mark de Berg, Otfried Cheong, Marc van Kreveld, Mark Overmars
                // Computational Geometry. Algorithms and Applications. Third Edition.
                // Theorem 11.1.
                size_t cnt = 2 * point_count - 4;
                return std::tuple<size_t, size_t>(cnt, cnt);
        }
        case 4:
        {
                // Handbook of Discrete and Computational Geometry edited by Jacob E. Goodman and Joseph O’Rourke.
                // Second edition.
                // 22.3 COMPUTING COMBINATORIAL DESCRIPTIONS.
                // Точно не определить, так как зависит от триангуляции.
                //
                // Опыты с выпуклой оболочкой со случайным распределением точек на четырёхмерной сфере
                // дают отношение количества граней к количеству точек около 6.7
                return std::tuple<size_t, size_t>(6.55 * point_count, 6.85 * point_count);
        }
        }
}

template <size_t N>
void test_algorithms(double rho, double alpha, const std::vector<Vector<N, float>>& points, size_t expected_facets_min,
                     size_t expected_facets_max, size_t expected_bound_facets_min, size_t expected_bound_facets_max)
{
        ASSERT(points.size() > N);
        ASSERT(expected_facets_min > 0 && expected_facets_max > 0 && expected_bound_facets_min > 0 &&
               expected_bound_facets_max > 0);

        double start_time = get_time_seconds();

        LOG("Point count: " + to_string(points.size()));

        std::string facet_count_str = (expected_facets_min == expected_facets_max) ?
                                              to_string(expected_facets_min) :
                                              "[" + to_string(expected_facets_min) + ", " + to_string(expected_facets_max) + "]";
        std::string bound_facet_count_str =
                (expected_bound_facets_min == expected_bound_facets_max) ?
                        to_string(expected_bound_facets_min) :
                        "[" + to_string(expected_bound_facets_min) + ", " + to_string(expected_bound_facets_max) + "]";

        LOG("Expected facet count: " + facet_count_str);
        LOG("Expected bound facet count: " + bound_facet_count_str);

        ProgressRatio progress(nullptr);

        std::unique_ptr<IManifoldConstructor<N>> sr = create_manifold_constructor(points, &progress);

        std::vector<Vector<N, double>> normals;
        std::vector<std::array<int, N>> facets;

        sr->cocone(&normals, &facets, &progress);

        LOG("COCONE facet count: " + to_string(facets.size()));
        if (!(expected_facets_min <= facets.size() && facets.size() <= expected_facets_max))
        {
                error("Error facet count: expected " + facet_count_str + ", COCONE computed " + to_string(facets.size()));
        }

        sr->bound_cocone(rho, alpha, &normals, &facets, &progress);

        LOG("BOUND COCONE facet count: " + to_string(facets.size()));
        if (!(expected_bound_facets_min <= facets.size() && facets.size() <= expected_bound_facets_max))
        {
                error("Error bound facet count: expected " + bound_facet_count_str + ", BOUND COCONE computed " +
                      to_string(facets.size()));
        }

        LOG("Time: " + to_string_fixed(get_time_seconds() - start_time, 5) + " s");
        LOG("Successful manifold reconstruction in " + to_string(N) + "D");
}

template <size_t N>
std::vector<Vector<N, float>> clone(const std::vector<Vector<N, float>>& points, size_t new_object_count, double shift)
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
                for (size_t i = 0; i < points.size(); ++i)
                {
                        clones.push_back(points[i] + vec_shift);
                }
        }

        ASSERT(clones.size() == points.size() * all_object_count);

        return clones;
}

template <size_t N>
void all_tests_unbound(int point_count)
{
        static_assert(2 <= N && N <= 4);
        ASSERT(point_count > 0);

        // BOUND COCONE может давать разные результаты в зависимости от точек и параметров,
        // поэтому надо проверять попадание в интервал, а не равенство
        constexpr double bound_low_coef = 0.9;
        constexpr double bound_high_coef = 1.1;

        // Объект находится в начале координат и имеет размеры не более 1 по каждой координате
        // в обе стороны, поэтому достаточно смещение на 3 для отсутствия пересечений объектов
        constexpr double shift = 3;

        std::vector<Vector<N, float>> points = create_object_repository<N>()->sphere_with_notch(point_count);

        auto[facets_min, facets_max] = facet_count<N>(points.size());
        size_t bound_facets_min = bound_low_coef * facets_min;
        size_t bound_facets_max = bound_high_coef * facets_max;

        LOG("------- " + to_string(N) + "D, 1 object -------");

        test_algorithms(RHO, ALPHA, points, facets_min, facets_max, bound_facets_min, bound_facets_max);

        LOG("");

        // Разместить вокруг объекта другие такие же объекты по каждой координате в обе стороны

        constexpr unsigned new_object_count = 1 << N;
        constexpr unsigned all_object_count = (1 + new_object_count);

        LOG("------- " + to_string(N) + "D, " + to_string(all_object_count) + " objects -------");

        test_algorithms(RHO, ALPHA, clone(points, new_object_count, shift), facets_min * all_object_count,
                        facets_max * all_object_count, bound_facets_min * all_object_count, bound_facets_max * all_object_count);
}

template <size_t N>
void test(int low, int high)
{
        try
        {
                thread_local std::mt19937_64 engine(get_random_seed<std::mt19937_64>());

                int point_count = std::uniform_int_distribution<int>(low, high)(engine);

                all_tests_unbound<N>(point_count);

                LOG("");
        }
        catch (std::exception& e)
        {
                error_fatal(std::string(to_string(N - 1) + "-manifold reconstruction test:\n") + e.what());
        }
}
}

void reconstruction_test(bool all)
{
        test<2>(100, 1000);

        test<3>(2000, 3000);

        if (!all)
        {
                return;
        }

        test<4>(20000, 25000);
}
