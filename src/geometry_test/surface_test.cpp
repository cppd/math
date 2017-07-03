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

#include "surface_test.h"

#include "points.h"

#include "com/log.h"
#include "com/math.h"
#include "com/print.h"
#include "com/random.h"
#include "geometry_cocone/surface.h"
#include "progress/progress.h"

#include <cmath>
#include <random>
#include <tuple>

// Для BOUND COCONE
constexpr double RHO = 0.3;
constexpr double ALPHA = 0.14;

// Надо располагать точки по целым числам
constexpr size_t Discretization = 100000;

namespace
{
template <size_t N>
std::tuple<size_t, size_t> facet_count(size_t point_count);

template <>
std::tuple<size_t, size_t> facet_count<2>(size_t point_count)
{
        size_t cnt = point_count;
        return std::tuple<size_t, size_t>(cnt, cnt);
}

template <>
std::tuple<size_t, size_t> facet_count<3>(size_t point_count)
{
        size_t cnt = 2 * point_count - 4;
        return std::tuple<size_t, size_t>(cnt, cnt);
}

template <>
std::tuple<size_t, size_t> facet_count<4>(size_t point_count)
{
        // Опыты с выпуклой оболочкой в 4D дают отношение количества
        // граней к количеству точек около 6.7
        return std::tuple<size_t, size_t>(6.55 * point_count, 6.85 * point_count);
}

template <size_t N>
void test(double rho, double alpha, const std::vector<Vector<N, float>>& points, size_t expected_facet_count_min,
          size_t expected_facet_count_max, size_t expected_bound_facet_count_min, size_t expected_bound_facet_count_max)
{
        ASSERT(points.size() > N);
        ASSERT(expected_facet_count_min > 0 && expected_facet_count_max > 0 && expected_bound_facet_count_min > 0 &&
               expected_bound_facet_count_max > 0);

        LOG("Point count: " + to_string(points.size()));

        std::string facet_count_str =
                (expected_facet_count_min == expected_facet_count_max) ?
                        to_string(expected_facet_count_min) :
                        "[" + to_string(expected_facet_count_min) + ", " + to_string(expected_facet_count_max) + "]";
        std::string bound_facet_count_str =
                (expected_bound_facet_count_min == expected_bound_facet_count_max) ?
                        to_string(expected_bound_facet_count_min) :
                        "[" + to_string(expected_bound_facet_count_min) + ", " + to_string(expected_bound_facet_count_max) + "]";

        LOG("Expected facet count: " + facet_count_str);
        LOG("Expected bound facet count: " + bound_facet_count_str);

        ProgressRatio progress(nullptr);

        std::unique_ptr<ISurfaceConstructor<N>> sr = create_surface_constructor(points, &progress);

        std::vector<Vector<N, double>> normals;
        std::vector<std::array<int, N>> facets;

        sr->cocone(&normals, &facets, &progress);

        LOG("COCONE facet count: " + to_string(facets.size()));
        if (!(expected_facet_count_min <= facets.size() && facets.size() <= expected_facet_count_max))
        {
                error("Error facet count: expected " + facet_count_str + ", COCONE computed " + to_string(facets.size()));
        }

        sr->bound_cocone(rho, alpha, &normals, &facets, &progress);

        LOG("BOUND COCONE facet count: " + to_string(facets.size()));
        if (!(expected_bound_facet_count_min <= facets.size() && facets.size() <= expected_bound_facet_count_max))
        {
                error("Error bound_facet count: expected " + bound_facet_count_str + ", BOUND_COCONE computed " +
                      to_string(facets.size()));
        }
}

template <size_t N>
std::vector<Vector<N, float>> clone(size_t new_object_count, double shift, const std::vector<Vector<N, float>>& points)
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
void all_tests_unbound(size_t size)
{
        static_assert(2 <= N && N <= 4);

        // BOUND COCONE может давать разные результаты в зависимости от точек и параметров,
        // поэтому надо проверять попадание в интервал, а не равенство
        constexpr double bound_low = 0.9;
        constexpr double bound_high = 1.1;

        // Объект находится в начале координат и имеет размеры не более 1 по каждой координате
        // в обе стороны, поэтому достаточно смещение на 3 для отсутствия пересечений объектов
        constexpr double shift = 3;

        std::vector<Vector<N, float>> points = generate_points_object_recess<N, Discretization>(size);

        auto[facet_count_min, facet_count_max] = facet_count<N>(points.size());

        //
        // Один объект

        LOG("------- " + to_string(N) + "D, 1 object -------");

        test(RHO, ALPHA, points, facet_count_min, facet_count_max, bound_low * facet_count_min, bound_high * facet_count_max);

        //
        // Разместить вокруг объекта другие такие же объекты по каждой координате в обе стороны

        // Для 4D только 2 дополнительных объекта, иначе получается много вычислений
        constexpr unsigned new_object_count = (N < 4) ? (1 << N) : 2;
        constexpr unsigned all_object_count = (1 + new_object_count);

        facet_count_min *= all_object_count;
        facet_count_max *= all_object_count;

        LOG("------- " + to_string(N) + "D, " + to_string(all_object_count) + " objects -------");

        points = clone(new_object_count, shift, points);

        test(RHO, ALPHA, points, facet_count_min, facet_count_max, bound_low * facet_count_min, bound_high * facet_count_max);

        LOG("Successful reconstruction in " + to_string(N) + "D");
}

void small_surface_test()
{
        try
        {
                std::mt19937_64 engine(get_random_seed<std::mt19937_64>());

                int size;

                size = std::uniform_int_distribution<int>(100, 1000)(engine);
                all_tests_unbound<2>(size);

                size = std::uniform_int_distribution<int>(2000, 3000)(engine);
                all_tests_unbound<3>(size);

                LOG("");
        }
        catch (std::exception& e)
        {
                error_fatal(std::string("manifold reconstruction test error: ") + e.what());
        }
}

void big_surface_test()
{
        try
        {
                std::mt19937_64 engine(get_random_seed<std::mt19937_64>());

                int size;

                size = std::uniform_int_distribution<int>(20000, 25000)(engine);
                all_tests_unbound<4>(size);

                LOG("");
        }
        catch (std::exception& e)
        {
                error_fatal(std::string("big manifold reconstruction test error: ") + e.what());
        }
}
}

void surface_test(bool all)
{
        small_surface_test();

        if (!all)
        {
                return;
        }

        big_surface_test();
}
