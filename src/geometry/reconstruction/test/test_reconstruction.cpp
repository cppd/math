/*
Copyright (C) 2017-2021 Topological Manifold

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

#include "../cocone.h"

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/names.h>
#include <src/com/print.h>
#include <src/com/random/engine.h>
#include <src/com/time.h>
#include <src/geometry/core/check.h>
#include <src/geometry/core/euler.h>
#include <src/sampling/sphere_uniform.h>
#include <src/test/test.h>

#include <cmath>
#include <random>
#include <tuple>

namespace ns::geometry
{
namespace
{
constexpr double BOUND_COCONE_RHO = 0.3;
constexpr double BOUND_COCONE_ALPHA = 0.14;

constexpr double COS_FOR_BOUND = -0.3;

template <typename T, std::size_t... I, typename V>
constexpr Vector<sizeof...(I) + 1, T> make_last_axis(V&& value, std::integer_sequence<std::size_t, I...>&&)
{
        return {(static_cast<void>(I), 0)..., std::forward<V>(value)};
}

template <std::size_t N, typename T>
constexpr Vector<N, T> LAST_AXIS = make_last_axis<T>(1, std::make_integer_sequence<std::size_t, N - 1>());

template <std::size_t N, typename T, typename RandomEngine>
Vector<N, T> random_on_sphere(RandomEngine& engine, bool bound)
{
        if (!bound)
        {
                return sampling::uniform_on_sphere<N, T>(engine);
        }
        Vector<N, T> v;
        do
        {
                v = sampling::uniform_on_sphere<N, T>(engine);
        } while (dot(v, LAST_AXIS<N, T>) < COS_FOR_BOUND);
        return v;
}

// Точки на сфере с углублением со стороны последней оси
// в положительном направлении этой оси
template <std::size_t N>
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

template <std::size_t N>
std::vector<Vector<N, float>> clone_object(const std::vector<Vector<N, float>>& points, unsigned clone_count)
{
        ASSERT(clone_count > 1 && clone_count <= (1 << N));

        // Объект находится в начале координат и имеет размеры не более 1 по каждой координате
        // в обе стороны, поэтому достаточно смещение на 3 для отсутствия пересечений объектов
        constexpr float SHIFT = 3;

        unsigned all_object_count = (1 + clone_count);

        std::vector<Vector<N, float>> clones(points.begin(), points.end());

        clones.reserve(points.size() * all_object_count);

        for (unsigned clone = 0; clone < clone_count; ++clone)
        {
                Vector<N, float> vec_shift;
                for (unsigned n = 0; n < N; ++n)
                {
                        vec_shift[n] = ((1 << n) & clone) ? SHIFT : -SHIFT;
                }
                for (unsigned i = 0; i < points.size(); ++i)
                {
                        clones.push_back(points[i] + vec_shift);
                }
        }

        ASSERT(clones.size() == points.size() * all_object_count);

        return clones;
}

template <std::size_t N>
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

template <typename T>
std::string min_max_to_string(T min, T max)
{
        if (min == max)
        {
                return to_string(min);
        }
        return "[" + to_string(min) + ", " + to_string(max) + "]";
}

template <std::size_t N>
void test_algorithms(
        bool bounded_object,
        unsigned object_count,
        const std::vector<Vector<N, float>>& points,
        ProgressRatio* progress)
{
        ASSERT(points.size() > N);
        ASSERT(object_count > 0);
        ASSERT(points.size() % object_count == 0);

        const auto [FACETS_MIN, FACETS_MAX] = facet_count<N>(points.size() / object_count);

        TimePoint start_time = time();

        LOG("Point count: " + to_string(points.size()));

        std::unique_ptr<ManifoldConstructor<N>> constructor = create_manifold_constructor(points, progress);

        if (!bounded_object)
        {
                // Задать размер для проверки очистки массивов
                std::vector<Vector<N, double>> normals(10000);
                std::vector<std::array<int, N>> facets(10000);

                unsigned expected_facets_min = FACETS_MIN * object_count;
                unsigned expected_facets_max = FACETS_MAX * object_count;
                std::string facet_count_str = min_max_to_string(expected_facets_min, expected_facets_max);

                LOG("Cocone expected facet count: " + facet_count_str);

                constructor->cocone(&normals, &facets, progress);

                LOG("Cocone facet count: " + to_string(facets.size()));
                if (!(expected_facets_min <= facets.size() && facets.size() <= expected_facets_max))
                {
                        error("Error facet count: expected " + facet_count_str + ", Cocone computed "
                              + to_string(facets.size()));
                }

                constexpr bool has_boundary = false;
                const int euler_characteristic = object_count * euler_characteristic_for_convex_polytope<N>();
                check_mesh("Cocone reconstruction", points, facets, has_boundary, euler_characteristic);
        }

        {
                // Задать размер для проверки очистки массивов
                std::vector<Vector<N, double>> normals(10000);
                std::vector<std::array<int, N>> facets(10000);

                // BoundCocone может давать разные результаты в зависимости от точек и параметров,
                // поэтому надо проверять попадание в интервал, а не равенство
                unsigned expected_facets_min = std::lround(0.9 * FACETS_MIN * object_count);
                unsigned expected_facets_max = std::lround(1.1 * FACETS_MAX * object_count);
                std::string facet_count_str = min_max_to_string(expected_facets_min, expected_facets_max);

                LOG("BoundCocone expected facet count: " + facet_count_str);

                constructor->bound_cocone(BOUND_COCONE_RHO, BOUND_COCONE_ALPHA, &normals, &facets, progress);

                LOG("BoundCocone facet count: " + to_string(facets.size()));
                if (!(expected_facets_min <= facets.size() && facets.size() <= expected_facets_max))
                {
                        error("Error facet count: expected " + facet_count_str + ", BoundCocone computed "
                              + to_string(facets.size()));
                }
        }

        LOG("Time: " + to_string_fixed(duration_from(start_time), 5) + " s");
        LOG("Successful manifold reconstruction in " + space_name(N));
}

template <std::size_t N>
void all_tests(bool bounded_object, std::vector<Vector<N, float>>&& points, ProgressRatio* progress)
{
        static_assert(2 <= N && N <= 4);

        LOG("------- " + space_name(N) + ", 1 object -------");
        test_algorithms(bounded_object, 1, points, progress);

        LOG("");

        // Размещение вокруг объекта других таких же объектов по каждой координате в обе стороны
        constexpr unsigned CLONE_COUNT = 1 << N;
        constexpr unsigned OBJECT_COUNT = (1 + CLONE_COUNT);
        LOG("------- " + space_name(N) + ", " + to_string(OBJECT_COUNT) + " objects -------");
        test_algorithms(bounded_object, OBJECT_COUNT, clone_object(points, CLONE_COUNT), progress);
}

template <std::size_t N>
void test(int low, int high, ProgressRatio* progress)
{
        const int POINT_COUNT = [&]()
        {
                std::mt19937 engine = create_engine<std::mt19937>();
                return std::uniform_int_distribution<int>(low, high)(engine);
        }();

        LOG("\n--- Unbound " + to_string(N - 1) + "-manifold reconstructions in " + space_name(N) + " ---\n");
        all_tests<N>(false, points_sphere_with_notch<N>(POINT_COUNT, false), progress);

        LOG("\n--- Bound " + to_string(N - 1) + "-manifold reconstructions in " + space_name(N) + " ---\n");
        all_tests<N>(true, points_sphere_with_notch<N>(POINT_COUNT, true), progress);
}

void test_reconstruction_2(ProgressRatio* progress)
{
        test<2>(100, 1000, progress);
}

void test_reconstruction_3(ProgressRatio* progress)
{
        test<3>(2000, 3000, progress);
}

void test_reconstruction_4(ProgressRatio* progress)
{
        test<4>(20000, 25000, progress);
}

TEST_SMALL("1-Manifold Reconstruction in 2-Space", test_reconstruction_2)
TEST_SMALL("2-Manifold Reconstruction in 3-Space", test_reconstruction_3)
TEST_LARGE("3-Manifold Reconstruction in 4-Space", test_reconstruction_4)
}
}
