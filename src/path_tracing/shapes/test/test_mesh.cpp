/*
Copyright (C) 2017, 2018 Topological Manifold

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

#include "test_mesh.h"

#include "com/error.h"
#include "com/log.h"
#include "com/print.h"
#include "com/random/engine.h"
#include "com/time.h"
#include "com/vec.h"
#include "geometry/core/convex_hull.h"
#include "obj/obj_facets.h"
#include "path_tracing/sampling/sphere.h"
#include "path_tracing/shapes/mesh.h"
#include "progress/progress.h"

#include <random>
#include <vector>

namespace
{
template <typename T, typename RandomEngine>
T random_integer(RandomEngine& random_engine, T low, T high)
{
        static_assert(std::is_integral_v<T>);
        ASSERT(low <= high);

        return std::uniform_int_distribution<T>(low, high)(random_engine);
}

template <typename T, typename RandomEngine, typename T1, typename T2>
T random_exponent_float(RandomEngine& random_engine, T1 exponent_low, T2 exponent_high)
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

template <size_t N, typename T, typename TS>
std::vector<Ray<N, T>> generate_random_rays_for_sphere(const Vector<N, TS>& center, TS radius, int count)
{
        LOG("random rays...");

        std::mt19937_64 random_engine(count);

        std::vector<Ray<N, T>> rays(count);

        Vector<N, T> v;
        T length_square;

        for (int i = 0; i < count; ++i)
        {
                random_in_sphere(random_engine, v, length_square);
                v /= std::sqrt(length_square);

                rays[i] = Ray<N, T>(T(2 * radius) * v + to_vector<T>(center), -v);
        }

        return rays;
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
                                                         int thread_count)
{
        ProgressRatio progress(nullptr);

        std::vector<Vector<N, float>> points;
        std::vector<std::array<int, N>> facets;

        LOG("convex hull in " + to_string(N) + "D, point count " + to_string(point_count));

        create_spherical_convex_hull(center, radius, point_count, &points, &facets, &progress);

        LOG("obj...");

        std::unique_ptr<const Obj<N>> obj(create_obj_for_facets(points, facets));

        LOG("simplex mesh...");

        constexpr Matrix<N + 1, N + 1, T> matrix(1);

        return std::make_unique<const Mesh<N, T>>(obj.get(), matrix, thread_count, &progress);
}

template <size_t N, typename T>
void test_mesh(T ray_offset, const Mesh<N, T>& mesh, const std::vector<Ray<N, T>>& rays, bool with_ray_log, bool with_error_log)
{
        int error_count = 0;

        LOG("intersections...");

        double start_time = time_in_seconds();

        for (unsigned i = 1; i <= rays.size(); ++i)
        {
                Ray<N, T> ray = rays[i - 1];

                if (with_ray_log)
                {
                        LOG("");
                        LOG("ray #" + to_string(i) + " in " + to_string(N) + "D");
                }

                T approximate, precise;
                const void* intersection_data;

                if (!mesh.intersect_approximate(ray, &approximate))
                {
                        if (with_error_log)
                        {
                                LOG("No the first approximate intersection with ray #" + to_string(i) + "\n" + to_string(ray));
                        }
                        ++error_count;
                        continue;
                }
                if (with_ray_log)
                {
                        LOG("t1_a == " + to_string(approximate));
                }
                if (!mesh.intersect_precise(ray, approximate, &precise, &intersection_data))
                {
                        if (with_error_log)
                        {
                                LOG("No the first precise intersection with ray #" + to_string(i) + "\n" + "approximate " +
                                    to_string(approximate) + "\n" + to_string(ray));
                        }
                        ++error_count;
                        continue;
                }
                if (with_ray_log)
                {
                        LOG("t1_p == " + to_string(precise));
                }

                ray.move_along_dir(precise + ray_offset);

                if (!mesh.intersect_approximate(ray, &approximate))
                {
                        if (with_error_log)
                        {
                                LOG("No the second approximate intersection with ray #" + to_string(i) + "\n" + to_string(ray));
                        }
                        ++error_count;
                        continue;
                }
                if (with_ray_log)
                {
                        LOG("t2_a == " + to_string(approximate));
                }
                if (!mesh.intersect_precise(ray, approximate, &precise, &intersection_data))
                {
                        if (with_error_log)
                        {
                                LOG("No the second precise intersection with ray #" + to_string(i) + "\n" + "approximate " +
                                    to_string(approximate) + "\n" + to_string(ray));
                        }
                        ++error_count;
                        continue;
                }
                if (with_ray_log)
                {
                        LOG("t2_p == " + to_string(precise));
                }

                ray.move_along_dir(precise + ray_offset);

                if (mesh.intersect_approximate(ray, &approximate) &&
                    mesh.intersect_precise(ray, approximate, &precise, &intersection_data))
                {
                        if (with_error_log)
                        {
                                LOG("The third intersection with ray #" + to_string(i) + "\n" + to_string(ray) + "\n" +
                                    "at point " + to_string(precise));
                        }
                        ++error_count;
                        continue;
                }
        }

        double error_percent = 100.0 * error_count / rays.size();

        LOG("intersections " + to_string_fixed(time_in_seconds() - start_time, 5) + " s");
        LOG("");
        LOG(to_string(error_count) + " errors, " + to_string(rays.size()) + " rays, " + to_string_fixed(error_percent, 5) + "%");
        LOG("");

        if (error_percent > 3)
        {
                error("Too many errors");
        }
}

template <size_t N, typename T>
void center_and_offset(float radius, Vector<N, float>* center, T* ray_offset)
{
        float center_float = -radius / 2;
        *center = Vector<N, float>(center_float);
        *ray_offset = (std::abs(center_float) + radius) * (100 * limits<T>::epsilon());
}

template <size_t N, typename T>
void test_mesh(float radius, int point_count, int ray_count, int thread_count, bool with_ray_log, bool with_error_log)
{
        LOG("----------- " + to_string(N) + "D, " + type_name<T>() + " -----------");

        Vector<N, float> center;
        T ray_offset;

        center_and_offset(radius, &center, &ray_offset);

        LOG("radius = " + to_string(radius));
        LOG("center = " + to_string(center));
        LOG("offset = " + to_string(ray_offset));

        std::unique_ptr<const Mesh<N, T>> mesh = simplex_mesh_of_sphere<N, T>(center, radius, point_count, thread_count);

        std::vector<Ray<N, T>> rays = generate_random_rays_for_sphere<N, T>(center, radius, ray_count);

        test_mesh(ray_offset, *mesh, rays, with_ray_log, with_error_log);
}

template <size_t N, typename T>
void test_mesh(int exponent_low, int exponent_high, int point_low, int point_high, int ray_low, int ray_high, int thread_count,
               bool with_ray_log, bool with_error_log)
{
        float radius;
        int point_count;
        int ray_count;

        {
                RandomEngineWithSeed<std::mt19937_64> random_engine;

                radius = random_exponent_float<float>(random_engine, exponent_low, exponent_high);
                point_count = random_integer(random_engine, point_low, point_high);
                ray_count = random_integer(random_engine, ray_low, ray_high);
        }

        test_mesh<N, T>(radius, point_count, ray_count, thread_count, with_ray_log, with_error_log);
}
}

void test_mesh()
{
        int thread_count = hardware_concurrency();

        bool with_ray_log = false;
        bool with_error_log = false;

        test_mesh<3, float>(-7, 10, 1000, 2000, 90'000, 110'000, thread_count, with_ray_log, with_error_log);
        test_mesh<3, double>(-22, 37, 1000, 2000, 90'000, 110'000, thread_count, with_ray_log, with_error_log);

        test_mesh<4, float>(-4, 6, 1000, 2000, 90'000, 110'000, thread_count, with_ray_log, with_error_log);
        test_mesh<4, double>(-22, 37, 1000, 2000, 90'000, 110'000, thread_count, with_ray_log, with_error_log);
}
