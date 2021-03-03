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

#include <src/com/log.h>
#include <src/com/names.h>
#include <src/com/random/engine.h>
#include <src/com/time.h>
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
std::vector<Vector<N, float>> generate_random_data(bool zero, int count, bool on_sphere)
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
void check_visible_from_point(const std::vector<Vector<N, float>>& points, const ConvexHullFacet<N>& facet, int point)
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
        if (!is_finite(d))
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
void check_convex_hull(const std::vector<Vector<N, float>>& points, std::vector<ConvexHullFacet<N>>* facets)
{
        LOG("Checking convex hull... ");

        {
                if (facets->empty())
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
                for (const ConvexHullFacet<N>& facet : *facets)
                {
                        array_facets.push_back(facet.vertices());
                }

                constexpr bool has_boundary = false;
                const int euler_characteristic = euler_characteristic_for_convex_polytope<N>();
                check_mesh("Convex hull in " + space_name(N), points, array_facets, has_boundary, euler_characteristic);
        }

        for (const ConvexHullFacet<N>& facet : *facets)
        {
                if (!is_finite(facet.ortho()))
                {
                        error("Facet ortho is not finite: " + to_string(facet.ortho()));
                }
                if (!(std::abs(1 - facet.ortho().norm()) < 1e-3))
                {
                        error("Facet ortho is not unit: " + to_string(facet.ortho().norm()));
                }

                for (unsigned i = 0; i < points.size(); ++i)
                {
                        check_visible_from_point(points, facet, i);
                }
        }

        LOG("Check passed");
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
std::vector<ConvexHullFacet<N>> create_convex_hull(std::vector<Vector<N, float>>& points, ProgressRatio* progress)
{
        std::vector<ConvexHullFacet<N>> facets;

        LOG("Convex hull...");
        TimePoint start_time = time();

        compute_convex_hull(points, &facets, progress, true);

        LOG("Convex hull created, " + to_string_fixed(duration_from(start_time), 5) + " s");
        LOG("Point count " + to_string(point_count(facets)) + ", facet count " + to_string(facets.size()));

        return facets;
}

template <std::size_t N>
void test(std::size_t low, std::size_t high, ProgressRatio* progress)
{
        constexpr bool ON_SPHERE = false;
        const int SIZE = [&]()
        {
                std::mt19937_64 engine = create_engine<std::mt19937_64>();
                return std::uniform_int_distribution<int>(low, high)(engine);
        }();

        {
                LOG("-----------------");
                constexpr bool ZERO = false;
                std::vector<Vector<N, float>> points = generate_random_data<N>(ZERO, SIZE, ON_SPHERE);
                LOG("Convex hull in " + space_name(N) + ", point count " + to_string(points.size()));
                std::vector<ConvexHullFacet<N>> facets = create_convex_hull(points, progress);
                check_convex_hull(points, &facets);
        }
        {
                LOG("-----------------");
                constexpr bool ZERO = true;
                std::vector<Vector<N, float>> points = generate_random_data<N>(ZERO, SIZE, ON_SPHERE);
                LOG("Convex hull in " + space_name(N) + ", point count " + to_string(points.size()));
                std::vector<ConvexHullFacet<N>> facets = create_convex_hull(points, progress);
                check_convex_hull(points, &facets);
        }
}

void test_performance()
{
        // При N=4, параллельно, 100000 точек, внутри сферы, примерное время: 1.7 сек, 0.4 сек.

        constexpr std::size_t N = 4;

        constexpr bool ON_SPHERE = false;
        constexpr int SIZE = 100000;

        // {{1, 1.000001}, {2, 3}, {2, 3}, {20, 3}, {4, 5}}

        ProgressRatio progress(nullptr);

        {
                LOG("-----------------");
                constexpr bool ZERO = false;
                std::vector<Vector<N, float>> points = generate_random_data<N>(ZERO, SIZE, ON_SPHERE);
                LOG("Convex hull, point count " + to_string(points.size()));
                create_convex_hull(points, &progress);
        }
        {
                LOG("-----------------");
                constexpr bool ZERO = true;
                std::vector<Vector<N, float>> points = generate_random_data<N>(ZERO, SIZE, ON_SPHERE);
                LOG("Convex hull, point count " + to_string(points.size()));
                create_convex_hull(points, &progress);
        }
}

void test_2(ProgressRatio* progress)
{
        test<2>(1000, 2000, progress);
}

void test_3(ProgressRatio* progress)
{
        test<3>(1000, 2000, progress);
}

void test_4(ProgressRatio* progress)
{
        test<4>(1000, 2000, progress);
}

void test_5(ProgressRatio* progress)
{
        test<5>(1000, 2000, progress);
}

TEST_SMALL("Convex Hull in 2-Space", test_2)
TEST_SMALL("Convex Hull in 3-Space", test_3)
TEST_SMALL("Convex Hull in 4-Space", test_4)
TEST_LARGE("Convex Hull in 5-Space", test_5)

TEST_PERFORMANCE("Convex Hull", test_performance)
}
}
