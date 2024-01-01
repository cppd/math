/*
Copyright (C) 2017-2024 Topological Manifold

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

#include "../convex_hull.h"

#include <src/com/chrono.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/names.h>
#include <src/com/print.h>
#include <src/com/random/pcg.h>
#include <src/com/string/str.h>
#include <src/geometry/core/check.h>
#include <src/geometry/core/euler.h>
#include <src/numerical/vector.h>
#include <src/progress/progress.h>
#include <src/settings/dimensions.h>
#include <src/test/test.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <random>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace ns::geometry::core
{
namespace
{
template <std::size_t N>
std::vector<Vector<N, float>> random_points(const bool zero, const int count, const bool on_sphere)
{
        PCG engine(count);
        std::uniform_real_distribution<double> urd(-1.0, 1.0);

        std::vector<Vector<N, float>> points(count);

        for (Vector<N, float>& point : points)
        {
                Vector<N, double> v;
                v[N - 1] = 0;
                do
                {
                        for (std::size_t i = 0; i < ((zero) ? (N - 1) : N); ++i)
                        {
                                v[i] = urd(engine);
                        }
                } while (v.norm_squared() > 1);

                if (!on_sphere)
                {
                        point = to_vector<float>(v);
                }
                else
                {
                        point = to_vector<float>(v.normalized());
                }
        }

        if (zero)
        {
                points[count - 1][N - 1] = 1;
        }

        return points;
}

template <std::size_t N>
void check_visible_from_point(
        const std::vector<Vector<N, float>>& points,
        const ConvexHullSimplex<N>& facet,
        const std::size_t point)
{
        if (points[point] == points[facet.vertices()[0]])
        {
                return;
        }

        const Vector<N, double> v = to_vector<double>(points[point] - points[facet.vertices()[0]]).normalized();
        if (!is_finite(v))
        {
                error("Vector from facet to point is not finite: " + to_string(v));
        }

        const double d = dot(facet.ortho(), v);
        if (!std::isfinite(d))
        {
                error("Dot product between " + to_string(facet.ortho()) + " and " + to_string(v)
                      + " is not finite: " + to_string(d));
        }
        if (!(d < 0.01))
        {
                error("Angle between facet normal and direction to point is too large: cosine = " + to_string(d));
        }
}

template <std::size_t N>
void check_convex_hull_data(
        const std::vector<Vector<N, float>>& points,
        const std::vector<ConvexHullSimplex<N>>& facets)
{
        if (facets.empty())
        {
                error("Convex hull empty facets");
        }

        constexpr std::size_t MIN_POINT_COUNT = N + 1;
        if (points.size() < MIN_POINT_COUNT)
        {
                error("Convex hull point count " + to_string(points.size()) + " is less than minimum point count "
                      + to_string(MIN_POINT_COUNT));
        }
}

template <std::size_t N>
void check_convex_hull_mesh(
        const std::vector<Vector<N, float>>& points,
        const std::vector<ConvexHullSimplex<N>>& facets)
{
        std::vector<std::array<int, N>> array_facets;
        array_facets.reserve(facets.size());
        for (const ConvexHullSimplex<N>& facet : facets)
        {
                array_facets.push_back(facet.vertices());
        }

        constexpr bool HAS_BOUNDARY = false;
        const int euler_characteristic = euler_characteristic_for_convex_polytope<N>();
        check_mesh("Convex hull in " + space_name(N), points, array_facets, HAS_BOUNDARY, euler_characteristic);
}

template <std::size_t N>
void check_convex_hull_facets(
        const std::vector<Vector<N, float>>& points,
        const std::vector<ConvexHullSimplex<N>>& facets)
{
        for (const ConvexHullSimplex<N>& facet : facets)
        {
                if (!is_finite(facet.ortho()))
                {
                        error("Facet ortho is not finite: " + to_string(facet.ortho()));
                }
                if (!facet.ortho().is_unit())
                {
                        error("Facet ortho is not unit: " + to_string(facet.ortho().norm()));
                }

                for (std::size_t i = 0; i < points.size(); ++i)
                {
                        check_visible_from_point(points, facet, i);
                }
        }
}

template <std::size_t N>
void check_convex_hull(const std::vector<Vector<N, float>>& points, const std::vector<ConvexHullSimplex<N>>& facets)
{
        check_convex_hull_data(points, facets);
        check_convex_hull_mesh(points, facets);
        check_convex_hull_facets(points, facets);
}

template <std::size_t N>
int point_count(const std::vector<ConvexHullSimplex<N>>& facets)
{
        std::unordered_set<int> v;
        for (const ConvexHullSimplex<N>& f : facets)
        {
                for (const int p : f.vertices())
                {
                        v.insert(p);
                }
        }
        return v.size();
}

template <std::size_t N>
std::vector<ConvexHullSimplex<N>> create_convex_hull(
        const std::vector<Vector<N, float>>& points,
        const bool write_log,
        const bool write_info,
        progress::Ratio* const progress)
{
        const Clock::time_point start_time = Clock::now();

        std::vector<ConvexHullSimplex<N>> res = compute_convex_hull(points, progress, write_log);

        const double time = duration_from(start_time);

        std::string s;
        s += "Convex hull in " + space_name(N) + ": time = " + to_string_fixed(time, 5) + " s";
        if (write_info)
        {
                s += ", source points = " + to_string_digit_groups(points.size());
                s += ", points = " + to_string_digit_groups(point_count(res));
                s += ", facets = " + to_string_digit_groups(res.size());
        }
        LOG(s);

        return res;
}

template <std::size_t N>
void test_convex_hull(progress::Ratio* const progress)
{
        constexpr int MIN_SIZE = 1'000;
        constexpr int MAX_SIZE = 2'000;
        constexpr bool ON_SPHERE = false;
        constexpr bool WRITE_LOG = true;
        constexpr bool WRITE_INFO = true;

        const int size = [&]()
        {
                PCG engine;
                return std::uniform_int_distribution<int>(MIN_SIZE, MAX_SIZE)(engine);
        }();

        // {{1, 1.000001}, {2, 3}, {2, 3}, {20, 3}, {4, 5}}

        const std::string name = "Test convex hull in " + space_name(N);
        LOG(name);
        {
                constexpr bool ZERO = false;
                const std::vector<Vector<N, float>> points = random_points<N>(ZERO, size, ON_SPHERE);
                const std::vector<ConvexHullSimplex<N>> facets =
                        create_convex_hull(points, WRITE_LOG, WRITE_INFO, progress);
                check_convex_hull(points, facets);
        }
        {
                constexpr bool ZERO = true;
                const std::vector<Vector<N, float>> points = random_points<N>(ZERO, size, ON_SPHERE);
                const std::vector<ConvexHullSimplex<N>> facets =
                        create_convex_hull(points, WRITE_LOG, WRITE_INFO, progress);
                check_convex_hull(points, facets);
        }
        LOG(name + " passed");
}

template <std::size_t N>
void test_performance(progress::Ratio* const progress)
{
        // N = 4, in parallel, 100000 points, inside sphere, time: 1.7 s, 0.4 s.

        constexpr bool ON_SPHERE = false;
        constexpr int SIZE = 100'000;
        constexpr bool WRITE_LOG = false;
        constexpr bool WRITE_INFO = false;

        {
                constexpr bool ZERO = false;
                create_convex_hull(random_points<N>(ZERO, SIZE, ON_SPHERE), WRITE_LOG, WRITE_INFO, progress);
        }
        {
                constexpr bool ZERO = true;
                create_convex_hull(random_points<N>(ZERO, SIZE, ON_SPHERE), WRITE_LOG, WRITE_INFO, progress);
        }
}

void performance_tests(progress::Ratio* const progress)
{
        [progress]<std::size_t... I>(std::index_sequence<I...>&&)
        {
                (test_performance<I>(progress), ...);
        }(settings::Dimensions2A());
}

template <std::size_t... I>
auto convex_hull_tests(std::index_sequence<I...>&&) noexcept
{
        return std::to_array({std::make_tuple(
                I <= 4 ? ns::test::Type::SMALL : ns::test::Type::LARGE,
                "Convex Hull, " + to_upper_first_letters(space_name(I)), test_convex_hull<I>)...});
}

TESTS(convex_hull_tests(settings::Dimensions2A()))

TEST_PERFORMANCE("Convex Hull", performance_tests)
}
}
