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
template <size_t N, typename T>
void generate_random_points_on_unit_sphere(int count, std::vector<Vector<N, T>>* points)
{
        std::mt19937_64 random_engine(count);

        points->resize(count);

        Vector<N, T> v;
        T length_square;

        for (int i = 0; i < count; ++i)
        {
                random_in_sphere(random_engine, v, length_square);
                (*points)[i] = v / std::sqrt(length_square);
        }
}

template <size_t N>
void create_spherical_convex_hull(int point_count, std::vector<Vector<N, float>>* points, std::vector<std::array<int, N>>* facets,
                                  ProgressRatio* progress)
{
        generate_random_points_on_unit_sphere(point_count, points);

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
                facets->push_back(ch_facet.get_vertices());
        }
}

template <size_t N, typename T>
std::unique_ptr<const Mesh<N, T>> create_simplex_mesh_of_unit_sphere(int point_count, int thread_count)
{
        ProgressRatio progress(nullptr);

        std::vector<Vector<N, float>> points;
        std::vector<std::array<int, N>> facets;

        LOG("convex hull in " + to_string(N) + "D, point count " + to_string(point_count));

        create_spherical_convex_hull(point_count, &points, &facets, &progress);

        LOG("obj...");

        std::unique_ptr<const Obj<N>> obj(create_obj_for_facets(points, facets));

        LOG("simplex mesh...");

        constexpr Matrix<N + 1, N + 1, T> matrix(1);

        return std::make_unique<const Mesh<N, T>>(obj.get(), matrix, thread_count, &progress);
}

template <size_t N, typename T, typename RandomEngine>
std::vector<Ray<N, T>> random_rays_for_unit_sphere(RandomEngine& random_engine, int ray_count)
{
        std::vector<Ray<N, T>> rays(ray_count);

        for (int i = 0; i < ray_count; ++i)
        {
                Vector<N, T> v;
                T length_square;

                random_in_sphere(random_engine, v, length_square);
                v /= std::sqrt(length_square);

                rays[i] = Ray<N, T>(T(11) * v, -v);
        }

        return rays;
}

template <size_t N, typename T, typename RandomEngine>
void test_mesh(RandomEngine random_engine, const Mesh<N, T>* mesh, int ray_count, bool with_ray_log)
{
        int error_count = 0;

        LOG("random rays..");

        std::vector<Ray<N, T>> rays = random_rays_for_unit_sphere<N, T>(random_engine, ray_count);

        LOG("intersections..");

        double start_time = time_in_seconds();

        for (int i = 1; i <= ray_count; ++i)
        {
                Ray<N, T>& ray = rays[i - 1];

                if (with_ray_log)
                {
                        LOG("");
                        LOG("ray #" + to_string(i) + " in " + to_string(N) + "D");
                }

                T approximate, precise;
                const void* intersection_data;

                if (!mesh->intersect_approximate(ray, &approximate))
                {
                        LOG("No the first approximate intersection with ray #" + to_string(i) + "\n" + to_string(ray));
                        ++error_count;
                        continue;
                }
                if (with_ray_log)
                {
                        LOG("t1_a == " + to_string(approximate));
                }
                if (!mesh->intersect_precise(ray, approximate, &precise, &intersection_data))
                {
                        LOG("No the first precise intersection with ray #" + to_string(i) + "\n" + "approximate " +
                            to_string(approximate) + "\n" + to_string(ray));
                        ++error_count;
                        continue;
                }
                if (with_ray_log)
                {
                        LOG("t1_p == " + to_string(precise));
                }

                ray.set_org(ray.point(precise));

                if (!mesh->intersect_approximate(ray, &approximate))
                {
                        LOG("No the second approximate intersection with ray #" + to_string(i) + "\n" + to_string(ray));
                        ++error_count;
                        continue;
                }
                if (with_ray_log)
                {
                        LOG("t2_a == " + to_string(approximate));
                }
                if (!mesh->intersect_precise(ray, approximate, &precise, &intersection_data))
                {
                        LOG("No the second precise intersection with ray #" + to_string(i) + "\n" + "approximate " +
                            to_string(approximate) + "\n" + to_string(ray));
                        ++error_count;
                        continue;
                }
                if (with_ray_log)
                {
                        LOG("t2_p == " + to_string(precise));
                }

                ray.set_org(ray.point(precise));

                if (mesh->intersect_approximate(ray, &approximate) &&
                    mesh->intersect_precise(ray, approximate, &precise, &intersection_data))
                {
                        LOG("The third intersection with ray #" + to_string(i) + "\n" + to_string(ray) + "\n" + "at point " +
                            to_string(precise));
                        ++error_count;
                        continue;
                }
        }

        LOG("intersections " + to_string_fixed(time_in_seconds() - start_time, 5) + " s");

        LOG(to_string(error_count) + " errors, " + to_string(ray_count) + " rays, " +
            to_string_fixed(100.0 * error_count / ray_count, 5) + "%");
}

template <size_t N, typename T>
void test_mesh(int point_low, int point_high, int ray_count, int thread_count, bool with_ray_log)
{
        LOG("----------- " + to_string(N) + "D -----------");

        RandomEngineWithSeed<std::mt19937_64> random_engine;

        int point_count = std::uniform_int_distribution<int>(point_low, point_high)(random_engine);

        std::unique_ptr<const Mesh<N, T>> mesh = create_simplex_mesh_of_unit_sphere<N, T>(point_count, thread_count);

        test_mesh(random_engine, mesh.get(), ray_count, with_ray_log);
}
}

void test_mesh()
{
        int hardware_concurrency = get_hardware_concurrency();

        bool with_ray_log = false;

        test_mesh<3, double>(1000, 2000, 1e5, hardware_concurrency, with_ray_log);
        test_mesh<4, double>(1000, 2000, 1e5, hardware_concurrency, with_ray_log);
}
