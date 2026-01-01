/*
Copyright (C) 2017-2026 Topological Manifold

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

#include <src/com/chrono.h>
#include <src/com/error.h>
#include <src/com/exponent.h>
#include <src/com/log.h>
#include <src/com/names.h>
#include <src/com/print.h>
#include <src/com/random/pcg.h>
#include <src/geometry/core/check.h>
#include <src/geometry/core/euler.h>
#include <src/geometry/reconstruction/cocone.h>
#include <src/numerical/vector.h>
#include <src/progress/progress.h>
#include <src/sampling/sphere_uniform.h>
#include <src/test/test.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <memory>
#include <random>
#include <string>
#include <tuple>
#include <vector>

namespace ns::geometry::reconstruction
{
namespace
{
constexpr double BOUND_COCONE_RHO = 0.3;
constexpr double BOUND_COCONE_ALPHA = 0.14;

constexpr double LAST_AXIS_VALUE = -0.3;

template <std::size_t N, typename T>
constexpr T last_axis(const numerical::Vector<N, T>& v)
{
        return v[N - 1];
}

template <std::size_t N, typename T, typename RandomEngine>
numerical::Vector<N, T> bound_uniform_on_sphere(RandomEngine& engine)
{
        numerical::Vector<N, T> res;
        do
        {
                res = sampling::uniform_on_sphere<N, T>(engine);
        } while (last_axis(res) < LAST_AXIS_VALUE);
        return res;
}

template <std::size_t N, typename T, typename RandomEngine>
numerical::Vector<N, T> uniform_on_sphere(RandomEngine& engine, const bool bound)
{
        if (!bound)
        {
                return sampling::uniform_on_sphere<N, T>(engine);
        }
        return bound_uniform_on_sphere<N, T>(engine);
}

template <std::size_t N>
std::vector<numerical::Vector<N, float>> points_sphere_with_notch(const unsigned point_count, const bool bound)
{
        PCG engine(point_count);

        std::vector<numerical::Vector<N, float>> res;
        res.reserve(point_count);
        for (unsigned i = 0; i < point_count; ++i)
        {
                numerical::Vector<N, double> v = uniform_on_sphere<N, double>(engine, bound);
                const double cos = last_axis(v);
                if (cos > 0)
                {
                        v[N - 1] *= 1 - std::abs(0.5 * power<5>(cos));
                }
                res.push_back(to_vector<float>(v));
        }
        return res;
}

template <std::size_t N>
std::vector<numerical::Vector<N, float>> clone_object(
        const std::vector<numerical::Vector<N, float>>& points,
        const unsigned clone_count)
{
        ASSERT(clone_count > 1 && clone_count <= (1 << N));

        // object has size 1 and is at the origin,
        // so shift by 3 to avoid intersection
        constexpr float SHIFT = 3;

        const unsigned all_object_count = (1 + clone_count);

        std::vector<numerical::Vector<N, float>> res(points.begin(), points.end());
        res.reserve(points.size() * all_object_count);
        for (unsigned clone = 0; clone < clone_count; ++clone)
        {
                numerical::Vector<N, float> shift;
                for (std::size_t i = 0; i < N; ++i)
                {
                        shift[i] = ((1 << i) & clone) ? SHIFT : -SHIFT;
                }
                for (std::size_t i = 0; i < points.size(); ++i)
                {
                        res.push_back(points[i] + shift);
                }
        }
        ASSERT(res.size() == points.size() * all_object_count);
        return res;
}

template <std::size_t N>
constexpr std::tuple<unsigned, unsigned> facet_count(const unsigned point_count)
{
        if constexpr (N == 2)
        {
                ASSERT(point_count >= 3);
                return {point_count, point_count};
        }
        else if constexpr (N == 3)
        {
                ASSERT(point_count >= 4);
                // Mark de Berg, Otfried Cheong, Marc van Kreveld, Mark Overmars
                // Computational Geometry. Algorithms and Applications. Third Edition.
                // Theorem 11.1.
                const unsigned count = 2 * point_count - 4;
                return {count, count};
        }
        else if constexpr (N == 4)
        {
                ASSERT(point_count >= 5);
                // Handbook of Discrete and Computational Geometry edited
                // by Jacob E. Goodman and Joseph O’Rourke. Second edition.
                // 22.3 COMPUTING COMBINATORIAL DESCRIPTIONS.
                // Some experiments (the convex hull of random points on a sphere)
                // show that it is about 6.7
                const unsigned min = std::lround(6.55 * point_count);
                const unsigned max = std::lround(6.85 * point_count);
                return {min, max};
        }
        else
        {
                static_assert(false);
        }
}

template <typename T>
std::string min_max_to_string(const T min, const T max)
{
        if (min == max)
        {
                return to_string(min);
        }
        return '[' + to_string(min) + ", " + to_string(max) + ']';
}

template <std::size_t N>
void test_normals(const std::vector<numerical::Vector<N, float>>& points, const ManifoldConstructor<N>& constructor)
{
        const std::vector<numerical::Vector<N, double>> normals = constructor.normals();
        if (normals.size() != points.size())
        {
                error("Error normal count: expected " + to_string(points.size()) + ", computed "
                      + to_string(normals.size()));
        }
}

template <std::size_t N>
void test_objects(
        const unsigned object_count,
        const std::vector<numerical::Vector<N, float>>& points,
        const ManifoldConstructor<N>& constructor,
        progress::Ratio* const progress)
{
        ASSERT(points.size() % object_count == 0);

        const auto [facets_min, facets_max] = facet_count<N>(points.size() / object_count);
        const unsigned expected_facets_min = facets_min * object_count;
        const unsigned expected_facets_max = facets_max * object_count;

        const std::string facet_count_str = min_max_to_string(expected_facets_min, expected_facets_max);

        LOG("Cocone expected facet count: " + facet_count_str);

        const std::vector<std::array<int, N>> facets = constructor.cocone(progress);

        LOG("Cocone facet count: " + to_string(facets.size()));
        if (!(expected_facets_min <= facets.size() && facets.size() <= expected_facets_max))
        {
                error("Error facet count: expected " + facet_count_str + ", Cocone computed "
                      + to_string(facets.size()));
        }

        constexpr bool HAS_BOUNDARY = false;
        const int euler_characteristic = object_count * core::euler_characteristic_for_convex_polytope<N>();
        core::check_mesh("Cocone reconstruction", points, facets, HAS_BOUNDARY, euler_characteristic);
}

template <std::size_t N>
void test_bound_objects(
        const unsigned object_count,
        const std::vector<numerical::Vector<N, float>>& points,
        const ManifoldConstructor<N>& constructor,
        progress::Ratio* const progress)
{
        ASSERT(points.size() % object_count == 0);

        const auto [facets_min, facets_max] = facet_count<N>(points.size() / object_count);
        const unsigned expected_facets_min = std::llround(0.9 * facets_min * object_count);
        const unsigned expected_facets_max = std::llround(1.1 * facets_max * object_count);

        const std::string facet_count_str = min_max_to_string(expected_facets_min, expected_facets_max);

        LOG("BoundCocone expected facet count: " + facet_count_str);

        const std::vector<std::array<int, N>> facets =
                constructor.bound_cocone(BOUND_COCONE_RHO, BOUND_COCONE_ALPHA, progress);

        LOG("BoundCocone facet count: " + to_string(facets.size()));
        if (!(expected_facets_min <= facets.size() && facets.size() <= expected_facets_max))
        {
                error("Error facet count: expected " + facet_count_str + ", BoundCocone computed "
                      + to_string(facets.size()));
        }
}

template <std::size_t N>
void test_algorithms(
        const bool bound_object,
        const unsigned object_count,
        const std::vector<numerical::Vector<N, float>>& points,
        progress::Ratio* const progress)
{
        ASSERT(points.size() > N);
        ASSERT(object_count > 0);
        ASSERT(points.size() % object_count == 0);

        const Clock::time_point start_time = Clock::now();

        LOG("Point count: " + to_string(points.size()));

        const std::unique_ptr<const ManifoldConstructor<N>> constructor = create_manifold_constructor(points, progress);

        test_normals(points, *constructor);

        if (!bound_object)
        {
                test_objects(object_count, points, *constructor, progress);
        }

        test_bound_objects(object_count, points, *constructor, progress);

        LOG("Manifold reconstruction in " + space_name(N) + ": " + to_string_fixed(duration_from(start_time), 5)
            + " s");
}

template <std::size_t N>
void all_tests(
        const bool bound_object,
        const std::vector<numerical::Vector<N, float>>& points,
        progress::Ratio* const progress)
{
        static_assert(2 <= N && N <= 4);

        LOG("------- " + space_name(N) + ", 1 object -------");
        test_algorithms(bound_object, 1, points, progress);

        LOG("");

        constexpr std::size_t CLONE_COUNT = 1 << N;
        constexpr std::size_t OBJECT_COUNT = (1 + CLONE_COUNT);
        LOG("------- " + space_name(N) + ", " + to_string(OBJECT_COUNT) + " objects -------");
        test_algorithms(bound_object, OBJECT_COUNT, clone_object(points, CLONE_COUNT), progress);
}

template <std::size_t N>
void test(const int low, const int high, progress::Ratio* const progress)
{
        const int point_count = [&]
        {
                PCG engine;
                return std::uniform_int_distribution<int>(low, high)(engine);
        }();

        LOG("\n--- Unbound " + to_string(N - 1) + "-manifold reconstructions in " + space_name(N) + " ---\n");
        all_tests<N>(false, points_sphere_with_notch<N>(point_count, false), progress);

        LOG("\n--- Bound " + to_string(N - 1) + "-manifold reconstructions in " + space_name(N) + " ---\n");
        all_tests<N>(true, points_sphere_with_notch<N>(point_count, true), progress);
}

void test_reconstruction_2(progress::Ratio* const progress)
{
        test<2>(100, 1000, progress);
}

void test_reconstruction_3(progress::Ratio* const progress)
{
        test<3>(2000, 3000, progress);
}

void test_reconstruction_4(progress::Ratio* const progress)
{
        test<4>(20000, 25000, progress);
}

TEST_SMALL("1-Manifold Reconstruction, 2-Space", test_reconstruction_2)
TEST_SMALL("2-Manifold Reconstruction, 3-Space", test_reconstruction_3)
TEST_LARGE("3-Manifold Reconstruction, 4-Space", test_reconstruction_4)
}
}
