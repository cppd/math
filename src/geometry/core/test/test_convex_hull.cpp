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

#include "../convex_hull.h"

#include <src/com/chrono.h>
#include <src/com/log.h>
#include <src/com/names.h>
#include <src/com/random/engine.h>
#include <src/geometry/core/check.h>
#include <src/geometry/core/euler.h>
#include <src/test/test.h>

#include <random>
#include <unordered_set>

namespace ns::geometry
{
namespace
{
template <std::size_t N>
std::vector<Vector<N, float>> random_data(const bool zero, const int count, const bool on_sphere)
{
        std::mt19937_64 engine(count);
        std::uniform_real_distribution<double> urd(-1.0, 1.0);

        std::vector<Vector<N, float>> points;
        points.resize(count);

        for (Vector<N, float>& point : points)
        {
                Vector<N, double> v;
                v[N - 1] = 0;
                do
                {
                        for (unsigned i = 0; i < ((zero) ? (N - 1) : N); ++i)
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
        const ConvexHullFacet<N>& facet,
        const int point)
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
void check_convex_hull(const std::vector<Vector<N, float>>& points, const std::vector<ConvexHullFacet<N>>& facets)
{
        {
                if (facets.empty())
                {
                        error("Convex hull empty facets");
                }

                constexpr unsigned MIN_POINT_COUNT = N + 1;
                if (points.size() < MIN_POINT_COUNT)
                {
                        error("Convex hull point count " + to_string(points.size())
                              + " is less than minimum point count " + to_string(MIN_POINT_COUNT));
                }
        }

        {
                std::vector<std::array<int, N>> array_facets;
                for (const ConvexHullFacet<N>& facet : facets)
                {
                        array_facets.push_back(facet.vertices());
                }

                constexpr bool HAS_BOUNDARY = false;
                const int euler_characteristic = euler_characteristic_for_convex_polytope<N>();
                check_mesh("Convex hull in " + space_name(N), points, array_facets, HAS_BOUNDARY, euler_characteristic);
        }

        for (const ConvexHullFacet<N>& facet : facets)
        {
                if (!is_finite(facet.ortho()))
                {
                        error("Facet ortho is not finite: " + to_string(facet.ortho()));
                }
                if (!facet.ortho().is_unit())
                {
                        error("Facet ortho is not unit: " + to_string(facet.ortho().norm()));
                }

                for (unsigned i = 0; i < points.size(); ++i)
                {
                        check_visible_from_point(points, facet, i);
                }
        }
}

template <std::size_t N>
int point_count(const std::vector<ConvexHullFacet<N>>& facets)
{
        std::unordered_set<int> v;
        for (const ConvexHullFacet<N>& f : facets)
        {
                for (int p : f.vertices())
                {
                        v.insert(p);
                }
        }
        return v.size();
}

template <std::size_t N>
std::vector<ConvexHullFacet<N>> create_convex_hull(
        const std::vector<Vector<N, float>>& points,
        const bool write_log,
        const bool write_info,
        ProgressRatio* const progress)
{
        std::vector<ConvexHullFacet<N>> facets;

        const Clock::time_point start_time = Clock::now();
        compute_convex_hull(points, &facets, progress, write_log);
        const double time = duration_from(start_time);

        std::string s;
        s += "Convex hull: time = " + to_string_fixed(time, 5) + " s";
        if (write_info)
        {
                s += ", source points = " + to_string_digit_groups(points.size());
                s += ", points = " + to_string_digit_groups(point_count(facets));
                s += ", facets = " + to_string_digit_groups(facets.size());
        }
        LOG(s);

        return facets;
}

template <std::size_t N>
void test(const std::size_t low, const std::size_t high, ProgressRatio* const progress)
{
        constexpr bool ON_SPHERE = false;
        constexpr bool WRITE_LOG = true;
        constexpr bool WRITE_INFO = true;

        const int size = [&]()
        {
                std::mt19937_64 engine = create_engine<std::mt19937_64>();
                return std::uniform_int_distribution<int>(low, high)(engine);
        }();

        const std::string name = "Test convex hull in " + space_name(N);
        LOG(name);
        {
                constexpr bool ZERO = false;
                const std::vector<Vector<N, float>> points = random_data<N>(ZERO, size, ON_SPHERE);
                const std::vector<ConvexHullFacet<N>> facets =
                        create_convex_hull(points, WRITE_LOG, WRITE_INFO, progress);
                check_convex_hull(points, facets);
        }
        {
                constexpr bool ZERO = true;
                const std::vector<Vector<N, float>> points = random_data<N>(ZERO, size, ON_SPHERE);
                const std::vector<ConvexHullFacet<N>> facets =
                        create_convex_hull(points, WRITE_LOG, WRITE_INFO, progress);
                check_convex_hull(points, facets);
        }
        LOG(name + " passed");
}

void test_performance()
{
        // N = 4, in parallel, 100000 points, inside sphere, time: 1.7 s, 0.4 s.

        constexpr std::size_t N = 4;

        constexpr bool ON_SPHERE = false;
        constexpr int SIZE = 100'000;
        constexpr bool WRITE_LOG = false;
        constexpr bool WRITE_INFO = false;

        // {{1, 1.000001}, {2, 3}, {2, 3}, {20, 3}, {4, 5}}

        ProgressRatio progress(nullptr);

        create_convex_hull(random_data<N>(false /*zero*/, SIZE, ON_SPHERE), WRITE_LOG, WRITE_INFO, &progress);
        create_convex_hull(random_data<N>(true /*zero*/, SIZE, ON_SPHERE), WRITE_LOG, WRITE_INFO, &progress);
}

void test_2(ProgressRatio* const progress)
{
        test<2>(1000, 2000, progress);
}

void test_3(ProgressRatio* const progress)
{
        test<3>(1000, 2000, progress);
}

void test_4(ProgressRatio* const progress)
{
        test<4>(1000, 2000, progress);
}

void test_5(ProgressRatio* const progress)
{
        test<5>(1000, 2000, progress);
}

TEST_SMALL("Convex hull, 2-Space", test_2)
TEST_SMALL("Convex hull, 3-Space", test_3)
TEST_SMALL("Convex hull, 4-Space", test_4)

TEST_LARGE("Convex hull, 5-Space", test_5)

TEST_PERFORMANCE("Convex hull", test_performance)
}
}
