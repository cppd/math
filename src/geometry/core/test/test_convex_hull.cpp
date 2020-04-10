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

#include "test_convex_hull.h"

#include "../convex_hull.h"
#include "../ridge.h"

#include <src/com/log.h>
#include <src/com/names.h>
#include <src/com/time.h>
#include <src/utility/random/engine.h>

#include <random>
#include <unordered_map>
#include <unordered_set>

namespace geometry
{
namespace
{
constexpr double CHECK_EPSILON = 0.01;

template <size_t N>
using RidgeData = RidgeData2<ConvexHullFacet<N>>;
template <size_t N>
using RidgeMap = std::unordered_map<Ridge<N>, RidgeData<N>>;

template <size_t N>
void generate_random_data(bool zero, int count, std::vector<Vector<N, float>>* points, bool on_sphere)
{
        std::mt19937_64 gen(count);
        std::uniform_real_distribution<double> urd(-1.0, 1.0);

        points->resize(count);

        for (int v_i = 0; v_i < count; ++v_i)
        {
                vec<N> v;
                v[N - 1] = 0;
                do
                {
                        for (unsigned i = 0; i < ((zero) ? (N - 1) : N); ++i)
                        {
                                v[i] = urd(gen);
                        }
                } while (v.norm_squared() > 1);

                if (!on_sphere)
                {
                        (*points)[v_i] = to_vector<float>(v);
                }
                else
                {
                        (*points)[v_i] = to_vector<float>(v.normalized());
                }
        }
        if (zero)
        {
                (*points)[count - 1][N - 1] = 1;
        }
}

template <size_t N>
void check_visible_from_point(const std::vector<Vector<N, float>>& points, const ConvexHullFacet<N>& facet, int point)
{
        if (points[point] == points[facet.vertices()[0]])
        {
                return;
        }

        vec<N> v = to_vector<double>(points[point] - points[facet.vertices()[0]]).normalized();

        if (!is_finite(v))
        {
                error("vector point facet not finite");
        }

        if (!is_finite(facet.ortho()))
        {
                error("facet ortho not finite");
        }

        double d = dot(facet.ortho(), v);

        if (!is_finite(d))
        {
                error("dot not finite");
        }

        // некоторые точки могут исключаться при построении выпуклой оболочки
        // сама оболочка может строиться на случайно смещённых точках

        if (d > CHECK_EPSILON)
        {
                error("Error check created convex hull, dot = " + to_string(d));
        }
}

template <size_t N>
void check_convex_hull(const std::vector<Vector<N, float>>& points, std::vector<ConvexHullFacet<N>>* facets)
{
        if (points.size() < N + 1 || facets->empty())
        {
                error("Error point count or facet count");
        }

        RidgeMap<N> ridges;

        for (ConvexHullFacet<N>& facet : *facets)
        {
                add_to_ridges(facet, &ridges);
        }
        for (auto i = ridges.cbegin(); i != ridges.cend(); ++i)
        {
                if (i->second.size() != 2)
                {
                        error("Error ridge not full");
                }
        }

        for (const ConvexHullFacet<N>& facet : *facets)
        {
                for (unsigned i = 0; i < points.size(); ++i)
                {
                        check_visible_from_point(points, facet, i);
                }
        }
}

template <size_t N>
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

template <size_t N>
void create_convex_hull(std::vector<Vector<N, float>>& points, bool with_check, ProgressRatio* progress)
{
        std::vector<ConvexHullFacet<N>> facets;

        LOG("convex hull...");
        double start_time = time_in_seconds();

        compute_convex_hull(points, &facets, progress);

        LOG("convex hull created, " + to_string_fixed(time_in_seconds() - start_time, 5) + " s");
        LOG("point count " + to_string(point_count(facets)) + ", facet count " + to_string(facets.size()));

        if (with_check)
        {
                LOG("checking convex hull... ");
                check_convex_hull(points, &facets);
                LOG("check passed");
        }
}

template <size_t N>
void test(size_t low, size_t high, ProgressRatio* progress)
{
        constexpr bool on_sphere = false;

        RandomEngineWithSeed<std::mt19937_64> engine;
        int size = std::uniform_int_distribution<int>(low, high)(engine);

        {
                std::vector<Vector<N, float>> points;
                LOG("-----------------");
                generate_random_data(false, size, &points, on_sphere);
                LOG("Convex hull in " + space_name(N) + ", point count " + to_string(points.size()));
                create_convex_hull(points, true, progress);
        }
        {
                std::vector<Vector<N, float>> points;
                LOG("-----------------");
                generate_random_data(true, size, &points, on_sphere);
                LOG("Convex hull in " + space_name(N) + ", point count " + to_string(points.size()));
                create_convex_hull(points, true, progress);
        }
}
}

void test_convex_hull_speed()
{
        // При N=4, параллельно, 100000 точек, внутри сферы, примерное время: 1.7 сек, 0.4 сек.

        constexpr size_t N = 4;

        constexpr bool on_sphere = false;

        std::vector<Vector<N, float>> points; // {{1, 1.000001}, {2, 3}, {2, 3}, {20, 3}, {4, 5}};
        int size = 100000;

        ProgressRatio progress(nullptr);

        LOG("-----------------");
        generate_random_data(false, size, &points, on_sphere);
        LOG("Integer convex hull, point count " + to_string(points.size()));
        create_convex_hull(points, false, &progress);

        LOG("-----------------");
        generate_random_data(true, size, &points, on_sphere);
        LOG("Integer convex hull, point count " + to_string(points.size()));
        create_convex_hull(points, false, &progress);
}

void test_convex_hull(int number_of_dimensions, ProgressRatio* progress)
{
        ASSERT(progress);

        switch (number_of_dimensions)
        {
        case 2:
                test<2>(1000, 2000, progress);
                break;
        case 3:
                test<3>(1000, 2000, progress);
                break;
        case 4:
                test<4>(1000, 2000, progress);
                break;
        case 5:
                test<5>(1000, 2000, progress);
                break;
        default:
                error("Error convex hull test number of dimensions " + to_string(number_of_dimensions));
        }
}
}
