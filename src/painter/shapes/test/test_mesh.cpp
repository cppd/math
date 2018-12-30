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
#include "com/names.h"
#include "com/print.h"
#include "com/random/engine.h"
#include "com/time.h"
#include "com/type_name.h"
#include "com/vec.h"
#include "painter/sampling/sphere.h"
#include "painter/shapes/test/sphere_mesh.h"

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

                rays[i] = Ray<N, T>(T(radius) * v + to_vector<T>(center), -v);
        }

        return rays;
}

template <size_t N, typename T>
T max_coordinate_modulus(const Vector<N, T>& a, const Vector<N, T>& b)
{
        T all_max = limits<T>::lowest();
        for (unsigned i = 0; i < N; ++i)
        {
                all_max = std::max(all_max, std::max(std::abs(a[i]), std::abs(b[i])));
        }
        return all_max;
}

template <size_t N, typename T>
void offset_and_rays_for_sphere_mesh(const Mesh<N, T>& mesh, int ray_count, T* offset, std::vector<Ray<N, T>>* rays)
{
        Vector<N, T> min, max;

        mesh.min_max(&min, &max);

        *offset = max_coordinate_modulus(min, max) * (100 * limits<T>::epsilon());

        LOG("ray offset = " + to_string(*offset));

        Vector<N, T> center = min + (max - min) / T(2);
        // Немного сместить центр, чтобы лучи не проходили через центр дерева
        center *= 0.99;

        T radius = max_element((max - min) / T(2));
        // При работе со сферой, чтобы начала лучей точно находились вне сферы,
        // достаточно немного увеличить половину максимального расстояния
        radius *= 2;

        LOG("ray center = " + to_string(center));
        LOG("ray radius = " + to_string(radius));

        *rays = generate_random_rays_for_sphere<N, T>(center, radius, ray_count);
}

template <size_t N, typename T>
void test_sphere_mesh(const Mesh<N, T>& mesh, int ray_count, bool with_ray_log, bool with_error_log, ProgressRatio* progress)
{
        T ray_offset;
        std::vector<Ray<N, T>> rays;

        offset_and_rays_for_sphere_mesh(mesh, ray_count, &ray_offset, &rays);

        int error_count = 0;

        LOG("intersections...");
        progress->set_text("Rays: %v of %m");

        double start_time = time_in_seconds();

        for (unsigned i = 1; i <= rays.size(); ++i)
        {
                if ((i & 0xfff) == 0xfff)
                {
                        progress->set(i, rays.size());
                }

                Ray<N, T> ray = rays[i - 1];

                if (with_ray_log)
                {
                        LOG("");
                        LOG("ray #" + to_string(i) + " in " + space_name(N));
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

        if (error_percent > 0.05)
        {
                error("Too many errors");
        }
}
}

namespace
{
template <size_t N, typename T>
void test_mesh(int point_low, int point_high, int ray_low, int ray_high, int thread_count, bool with_ray_log, bool with_error_log,
               ProgressRatio* progress)
{
        LOG("----------- " + space_name(N) + ", " + type_name<T>() + " -----------");

        int point_count;
        int ray_count;

        {
                RandomEngineWithSeed<std::mt19937_64> random_engine;
                point_count = random_integer(random_engine, point_low, point_high);
                ray_count = random_integer(random_engine, ray_low, ray_high);
        }

        std::unique_ptr mesh = simplex_mesh_of_random_sphere<N, T>(point_count, thread_count, progress);

        test_sphere_mesh(*mesh, ray_count, with_ray_log, with_error_log, progress);
}
}

void test_mesh(int number_of_dimensions, ProgressRatio* progress)
{
        ASSERT(progress);

        int thread_count = hardware_concurrency();

        bool with_ray_log = false;
        bool with_error_log = false;

        switch (number_of_dimensions)
        {
        case 3:
                test_mesh<3, float>(500, 1000, 90'000, 110'000, thread_count, with_ray_log, with_error_log, progress);
                test_mesh<3, double>(500, 1000, 90'000, 110'000, thread_count, with_ray_log, with_error_log, progress);
                break;
        case 4:
                test_mesh<4, float>(500, 1000, 90'000, 110'000, thread_count, with_ray_log, with_error_log, progress);
                test_mesh<4, double>(500, 1000, 90'000, 110'000, thread_count, with_ray_log, with_error_log, progress);
                break;
        case 5:
                test_mesh<5, float>(1000, 2000, 90'000, 110'000, thread_count, with_ray_log, with_error_log, progress);
                test_mesh<5, double>(1000, 2000, 90'000, 110'000, thread_count, with_ray_log, with_error_log, progress);
                break;
        case 6:
                test_mesh<6, float>(1000, 2000, 90'000, 110'000, thread_count, with_ray_log, with_error_log, progress);
                test_mesh<6, double>(1000, 2000, 90'000, 110'000, thread_count, with_ray_log, with_error_log, progress);
                break;
        default:
                error("Error mesh test number of dimensions " + to_string(number_of_dimensions));
        }
}
