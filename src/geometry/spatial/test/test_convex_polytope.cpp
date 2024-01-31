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

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/random/pcg.h>
#include <src/com/type/limit.h>
#include <src/geometry/spatial/convex_polytope.h>
#include <src/geometry/spatial/hyperplane.h>
#include <src/numerical/ray.h>
#include <src/numerical/vector.h>
#include <src/sampling/sphere_uniform.h>
#include <src/test/test.h>

#include <array>
#include <cstddef>
#include <random>
#include <string>
#include <vector>

namespace ns::geometry::spatial::test
{
namespace
{
template <typename T>
constexpr T MIN_D = 1;

template <typename T>
constexpr T MAX_D = 10;

template <std::size_t N, typename T, typename RandomEngine>
ConvexPolytope<N, T> create_random_spherical_polytope(RandomEngine& engine)
{
        constexpr std::size_t COUNT = 10 * N;

        std::uniform_real_distribution<T> urd(MIN_D<T>, MAX_D<T>);

        std::vector<Hyperplane<N, T>> planes;
        planes.reserve(COUNT);
        for (std::size_t i = 0; i < COUNT; ++i)
        {
                planes.emplace_back(sampling::uniform_on_sphere<N, T>(engine), urd(engine));
        }

        return ConvexPolytope<N, T>{std::move(planes)};
}

template <std::size_t N, typename T, typename RandomEngine>
std::vector<numerical::Vector<N, T>> internal_points(const int count, RandomEngine& engine)
{
        numerical::Vector<N, T> v;
        T v_length_square;

        std::vector<numerical::Vector<N, T>> res;
        res.reserve(count);
        for (int i = 0; i < count; ++i)
        {
                sampling::uniform_in_sphere(engine, v, v_length_square);
                res.push_back(v * (MIN_D<T> / 2));
        }
        return res;
}

template <typename T>
std::string str(const T near, const T far)
{
        return "near = " + to_string(near) + ", far = " + to_string(far);
}

template <std::size_t N, typename T>
numerical::Ray<N, T> test_ray_internal_and_move_ray(
        const ConvexPolytope<N, T>& polytope,
        const numerical::Ray<N, T>& ray)
{
        T near = Limits<T>::max();
        T far = 0;

        if (polytope.intersect(ray, &near, &far))
        {
                error("Convex polytope intersection error, internal point: " + str(near, far));
        }

        near = 0;
        far = Limits<T>::max();

        if (!polytope.intersect(ray, &near, &far))
        {
                error("No convex polytope intersection found, internal point");
        }

        if (!(near == 0 && near < far && far < Limits<T>::max()))
        {
                error("Convex polytope intersection error, internal point: " + str(near, far));
        }

        return ray.moved(far * 2);
}

template <std::size_t N, typename T>
void test_ray_external(const ConvexPolytope<N, T>& polytope, const numerical::Ray<N, T>& ray)
{
        T near = 0;
        T far = Limits<T>::max();

        if (polytope.intersect(ray, &near, &far))
        {
                error("Convex polytope intersection, external point: " + str(near, far));
        }
}

template <std::size_t N, typename T>
std::array<T, 2> test_reversed_ray_external_intersection(
        const ConvexPolytope<N, T>& polytope,
        const numerical::Ray<N, T>& ray)
{
        T near = 0;
        T far = Limits<T>::max();

        if (!polytope.intersect(ray, &near, &far))
        {
                error("No convex polytope intersection found, reversed ray, external point");
        }

        if (!(near > 0 && near < far && far < Limits<T>::max()))
        {
                error("Convex polytope intersection error, reversed ray, external point: " + str(near, far));
        }

        return {near, far};
}

template <std::size_t N, typename T>
void test_reversed_ray_external_no_intersection(
        const ConvexPolytope<N, T>& polytope,
        const numerical::Ray<N, T>& ray,
        const T intersection_near,
        const T intersection_far)
{
        T near = 0;
        T far = intersection_near / 2;

        if (polytope.intersect(ray, &near, &far))
        {
                error("Convex polytope intersection, reversed ray, external point: " + str(near, far));
        }

        near = intersection_far * 2;
        far = Limits<T>::max();

        if (polytope.intersect(ray, &near, &far))
        {
                error("Convex polytope intersection, reversed ray, external point: " + str(near, far));
        }
}

template <std::size_t N, typename T, typename RandomEngine>
void test(const int point_count, RandomEngine& engine)
{
        const ConvexPolytope<N, T> polytope = create_random_spherical_polytope<N, T>(engine);

        for (const numerical::Vector<N, T>& point : internal_points<N, T>(point_count, engine))
        {
                numerical::Ray<N, T> ray(point, sampling::uniform_on_sphere<N, T>(engine));

                ray = test_ray_internal_and_move_ray(polytope, ray);

                test_ray_external(polytope, ray);

                ray = ray.reversed();

                const auto [near, far] = test_reversed_ray_external_intersection(polytope, ray);

                test_reversed_ray_external_no_intersection(polytope, ray, near, far);
        }
}

template <typename T, typename RandomEngine>
void test(const int point_count, RandomEngine& engine)
{
        test<2, T>(point_count, engine);
        test<3, T>(point_count, engine);
        test<4, T>(point_count, engine);
        test<5, T>(point_count, engine);
        test<6, T>(point_count, engine);
}

void test_convex_polytope()
{
        PCG engine;

        LOG("Test convex polytope");
        test<float>(10'000, engine);
        test<double>(10'000, engine);
        test<long double>(1'000, engine);
        LOG("Test convex polytope passed");
}

TEST_SMALL("Convex Polytope", test_convex_polytope)
}
}
