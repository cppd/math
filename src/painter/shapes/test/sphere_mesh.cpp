/*
Copyright (C) 2017-2019 Topological Manifold

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

#include "sphere_mesh.h"

#include "com/names.h"
#include "com/random/engine.h"
#include "com/time.h"
#include "geometry/core/convex_hull.h"
#include "obj/create/facets.h"
#include "painter/sampling/sphere.h"

#include <random>
#include <type_traits>
#include <vector>

namespace
{
template <typename T, typename RandomEngine, typename T1, typename T2>
T random_exponent(RandomEngine& random_engine, T1 exponent_low, T2 exponent_high)
{
        static_assert(std::is_floating_point_v<T>);
        ASSERT(exponent_low <= exponent_high);

        return std::pow(10, std::uniform_real_distribution<T>(exponent_low, exponent_high)(random_engine));
}

template <size_t N, typename T>
std::vector<Vector<N, T>> generate_random_points_on_sphere(const Vector<N, T>& center, T radius, int count)
{
        LOG("random points...");

        std::mt19937_64 random_engine(count);

        std::vector<Vector<N, T>> points(count);

        Vector<N, T> v;
        T length_square;

        for (int i = 0; i < count; ++i)
        {
                random_in_sphere(random_engine, v, length_square);
                v /= std::sqrt(length_square);
                points[i] = v * radius + center;
        }

        return points;
}

template <size_t N>
void create_spherical_convex_hull(const Vector<N, float>& center, float radius, int point_count,
                                  std::vector<Vector<N, float>>* points, std::vector<std::array<int, N>>* facets,
                                  ProgressRatio* progress)
{
        *points = generate_random_points_on_sphere<N, float>(center, radius, point_count);

        std::vector<ConvexHullFacet<N>> ch_facets;

        LOG("convex hull...");
        double start_time = time_in_seconds();
        compute_convex_hull(*points, &ch_facets, progress);
        LOG("convex hull created, " + to_string_fixed(time_in_seconds() - start_time, 5) + " s");
        LOG("facet count = " + to_string(ch_facets.size()));

        facets->clear();
        facets->reserve(ch_facets.size());
        for (const ConvexHullFacet<N>& ch_facet : ch_facets)
        {
                facets->push_back(ch_facet.vertices());
        }
}

template <size_t N, typename T>
std::unique_ptr<const Mesh<N, T>> simplex_mesh_of_sphere(const Vector<N, float>& center, float radius, int point_count,
                                                         int thread_count, ProgressRatio* progress)
{
        std::vector<Vector<N, float>> points;
        std::vector<std::array<int, N>> facets;

        LOG("convex hull in " + space_name(N) + ", point count " + to_string(point_count));

        progress->set_text("Data: %v of %m");

        create_spherical_convex_hull(center, radius, point_count, &points, &facets, progress);

        LOG("obj...");

        std::unique_ptr<const Obj<N>> obj(create_obj_for_facets(points, facets));

        LOG("simplex mesh...");

        constexpr Matrix<N + 1, N + 1, T> matrix(1);

        return std::make_unique<const Mesh<N, T>>(obj.get(), matrix, thread_count, progress);
}
}

template <size_t N, typename T>
std::unique_ptr<const Mesh<N, T>> simplex_mesh_of_random_sphere(int point_count, int thread_count, ProgressRatio* progress)
{
        static_assert(N >= 3 && N <= 6);
        static_assert(std::is_same_v<T, float> || std::is_same_v<T, double>);

        // От 3 измерений последовательно
        constexpr int exponents_for_float[4][2] = {{-7, 10}, {-4, 6}, {-3, 5}, {-2, 3}};
        constexpr int exponents_for_double[4][2] = {{-22, 37}, {-22, 37}, {-22, 37}, {-22, 30}};

        const int* exponents = nullptr;
        if (std::is_same_v<T, float>)
        {
                exponents = exponents_for_float[N - 3];
        }
        if (std::is_same_v<T, double>)
        {
                exponents = exponents_for_double[N - 3];
        }
        ASSERT(exponents);

        RandomEngineWithSeed<std::mt19937_64> random_engine;
        float radius = random_exponent<float>(random_engine, exponents[0], exponents[1]);

        Vector<N, float> center(-radius / 2);

        LOG("mesh radius = " + to_string(radius));
        LOG("mesh center = " + to_string(center));

        return simplex_mesh_of_sphere<N, T>(center, radius, point_count, thread_count, progress);
}

template std::unique_ptr<const Mesh<3, float>> simplex_mesh_of_random_sphere(int point_count, int thread_count,
                                                                             ProgressRatio* progress);
template std::unique_ptr<const Mesh<4, float>> simplex_mesh_of_random_sphere(int point_count, int thread_count,
                                                                             ProgressRatio* progress);
template std::unique_ptr<const Mesh<5, float>> simplex_mesh_of_random_sphere(int point_count, int thread_count,
                                                                             ProgressRatio* progress);
template std::unique_ptr<const Mesh<6, float>> simplex_mesh_of_random_sphere(int point_count, int thread_count,
                                                                             ProgressRatio* progress);

template std::unique_ptr<const Mesh<3, double>> simplex_mesh_of_random_sphere(int point_count, int thread_count,
                                                                              ProgressRatio* progress);
template std::unique_ptr<const Mesh<4, double>> simplex_mesh_of_random_sphere(int point_count, int thread_count,
                                                                              ProgressRatio* progress);
template std::unique_ptr<const Mesh<5, double>> simplex_mesh_of_random_sphere(int point_count, int thread_count,
                                                                              ProgressRatio* progress);
template std::unique_ptr<const Mesh<6, double>> simplex_mesh_of_random_sphere(int point_count, int thread_count,
                                                                              ProgressRatio* progress);
