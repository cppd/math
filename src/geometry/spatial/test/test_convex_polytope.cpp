/*
Copyright (C) 2017-2022 Topological Manifold

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

#include "../convex_polytope.h"

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/random/pcg.h>
#include <src/com/type/limit.h>
#include <src/sampling/sphere_uniform.h>
#include <src/test/test.h>

#include <random>
#include <vector>

namespace ns::geometry::spatial
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
        return ConvexPolytope<N, T>{planes};
}

template <std::size_t N, typename T, typename RandomEngine>
std::vector<Vector<N, T>> internal_points(const int count, RandomEngine& engine)
{
        Vector<N, T> v;
        T v_length_square;
        std::vector<Vector<N, T>> points;
        points.reserve(count);
        for (int i = 0; i < count; ++i)
        {
                sampling::uniform_in_sphere(engine, v, v_length_square);
                points.push_back(v * (MIN_D<T> / 2));
        }
        return points;
}

template <std::size_t N, typename T, typename RandomEngine>
void test_intersections(const ConvexPolytope<N, T>& convex_polytope, const int point_count, RandomEngine& engine)
{
        for (const Vector<N, T>& point : internal_points<N, T>(point_count, engine))
        {
                Ray<N, T> ray(point, sampling::uniform_on_sphere<N, T>(engine));

                T near = Limits<T>::max();
                T far = 0;

                if (convex_polytope.intersect(ray, &near, &far))
                {
                        error("Convex polytope intersection error, internal point: near = " + to_string(near)
                              + ", far = " + to_string(far));
                }

                near = 0;
                far = Limits<T>::max();

                if (!convex_polytope.intersect(ray, &near, &far))
                {
                        error("No convex polytope intersection found, internal point");
                }

                if (!(near == 0 && near < far && far < Limits<T>::max()))
                {
                        error("Convex polytope intersection error, internal point: near = " + to_string(near)
                              + ", far = " + to_string(far));
                }

                ray.move(far * 2);
                near = 0;
                far = Limits<T>::max();

                if (convex_polytope.intersect(ray, &near, &far))
                {
                        error("Convex polytope intersection, external point: near = " + to_string(near)
                              + ", far = " + to_string(far));
                }

                ray = ray.reversed();

                T r_near = 0;
                T r_far = Limits<T>::max();

                if (!convex_polytope.intersect(ray, &r_near, &r_far))
                {
                        error("No convex polytope intersection found, external point, reversed");
                }

                if (!(r_near > 0 && r_near < r_far && r_far < Limits<T>::max()))
                {
                        error("Convex polytope intersection error, external point, reversed: near = "
                              + to_string(r_near) + ", far = " + to_string(r_far));
                }

                near = 0;
                far = r_near / 2;

                if (convex_polytope.intersect(ray, &near, &far))
                {
                        error("Convex polytope intersection, external point, reversed: near = " + to_string(near)
                              + ", far = " + to_string(far));
                }

                near = r_far * 2;
                far = Limits<T>::max();

                if (convex_polytope.intersect(ray, &near, &far))
                {
                        error("Convex polytope intersection, external point, reversed: near = " + to_string(near)
                              + ", far = " + to_string(far));
                }
        }
}

template <std::size_t N, typename T, typename RandomEngine>
void test(const int point_count, RandomEngine& engine)
{
        const ConvexPolytope<N, T> polytope = create_random_spherical_polytope<N, T>(engine);
        test_intersections(polytope, point_count, engine);
}

template <typename T, typename RandomEngine>
void test(const int point_count, RandomEngine& engine)
{
        test<2, T>(point_count, engine);
        test<3, T>(point_count, engine);
        test<4, T>(point_count, engine);
        test<5, T>(point_count, engine);
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

TEST_SMALL("Convex polytope", test_convex_polytope)
}
}
