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

#include "parallelotope_points.h"

#include "../bounding_box.h"

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/random/engine.h>
#include <src/sampling/sphere_uniform.h>
#include <src/test/test.h>

#include <cmath>
#include <random>

namespace ns::geometry::spatial::test
{
namespace
{
template <typename T>
struct Test
{
        static constexpr BoundingBox<4, T> BOX{Vector<4, T>(1, -2, 3, -4), Vector<4, T>(-5, 6, -7, 8)};
        static_assert(BOX.min() == Vector<4, T>(-5, -2, -7, -4));
        static_assert(BOX.max() == Vector<4, T>(1, 6, 3, 8));
        static_assert(BOX.diagonal() == Vector<4, T>(6, 8, 10, 12));
        static_assert(BOX.center() == Vector<4, T>(-2, 2, -2, 2));
        static_assert(BOX.volume() == 5760);
        static_assert(BOX.surface() == 2736);

        static constexpr BoundingBox<4, T> BOX_MERGE_1 = []
        {
                BoundingBox<4, T> b(BOX);
                b.merge(Vector<4, T>(5, -5, 5, -5));
                return b;
        }();
        static_assert(BOX_MERGE_1.min() == Vector<4, T>(-5, -5, -7, -5));
        static_assert(BOX_MERGE_1.max() == Vector<4, T>(5, 6, 5, 8));

        static constexpr BoundingBox<4, T> BOX_MERGE_2 = []
        {
                BoundingBox<4, T> b(BOX);
                b.merge(BoundingBox<4, T>(Vector<4, T>(4, -3, 2, -1), Vector<4, T>(-4, 5, -6, 7)));
                return b;
        }();
        static_assert(BOX_MERGE_2.min() == Vector<4, T>(-5, -3, -7, -4));
        static_assert(BOX_MERGE_2.max() == Vector<4, T>(4, 6, 3, 8));

        static constexpr BoundingBox<4, T> BOX_POINT{Vector<4, T>(1, -2, 3, -4)};
        static_assert(BOX_POINT.min() == Vector<4, T>(1, -2, 3, -4));
        static_assert(BOX_POINT.max() == Vector<4, T>(1, -2, 3, -4));

        static constexpr BoundingBox<4, T> BOX_ARRAY{
                std::array{Vector<4, T>(1, -2, 3, -4), Vector<4, T>(-5, 6, -7, 8)}};
        static_assert(BOX_ARRAY.min() == Vector<4, T>(-5, -2, -7, -4));
        static_assert(BOX_ARRAY.max() == Vector<4, T>(1, 6, 3, 8));
};

template struct Test<float>;
template struct Test<double>;
template struct Test<long double>;

//

template <std::size_t N, typename T>
BoundingBox<N, T> create_random_box(std::mt19937_64& engine)
{
        std::uniform_real_distribution<T> urd(-5, 5);
        Vector<N, T> p1;
        Vector<N, T> p2;
        for (std::size_t i = 0; i < N; ++i)
        {
                do
                {
                        p1[i] = urd(engine);
                        p2[i] = urd(engine);
                } while (!(std::abs(p1[i] - p2[i]) >= T(0.5)));
        }
        return {p1, p2};
}

template <std::size_t N, typename T>
std::array<Vector<N, T>, N> box_vectors(const BoundingBox<N, T>& box)
{
        const Vector<N, T> diagonal = box.diagonal();
        std::array<Vector<N, T>, N> res;
        for (std::size_t i = 0; i < N; ++i)
        {
                for (std::size_t j = 0; j < N; ++j)
                {
                        res[i][j] = 0;
                }
                res[i][i] = diagonal[i];
        }
        return res;
}

template <std::size_t N, typename T>
bool is_on_box(const BoundingBox<N, T>& box, const Vector<N, T>& point, const T& precision)
{
        for (std::size_t i = 0; i < N; ++i)
        {
                if (std::abs(point[i] - box.min()[i]) < precision)
                {
                        return true;
                }
                if (std::abs(point[i] - box.max()[i]) < precision)
                {
                        return true;
                }
        }
        return false;
}

template <std::size_t N, typename T>
void test_intersections(
        const BoundingBox<N, T>& box,
        const int point_count,
        const T& precision,
        std::mt19937_64& engine)
{
        const T length = box.diagonal().norm();
        const T move_distance = 2 * length;
        const T move_min = length;
        const T move_max = 2 * length;

        for (const Vector<N, T>& point : internal_points(box.min(), box_vectors(box), point_count, engine))
        {
                const Ray<N, T> ray(point, sampling::uniform_on_sphere<N, T>(engine));

                {
                        const Ray<N, T> r = ray;
                        const auto t = box.intersect(r);
                        if (!t)
                        {
                                error("Ray must intersect, inside\nbox " + to_string(box) + "\nray " + to_string(r));
                        }
                        if (!(*t > 0 && *t < length))
                        {
                                error("Intersection out of bounding box, inside\ndistance = " + to_string(*t) + ", "
                                      + "max distance = " + to_string(length) + "\nbox " + to_string(box) + "\nray "
                                      + to_string(r));
                        }

                        const Vector<N, T> p = r.point(*t);
                        if (!is_on_box(box, p, precision))
                        {
                                error("Intersection out of bounding box, inside\nintersection point = " + to_string(p)
                                      + "\nbox " + to_string(box) + "\nray " + to_string(r));
                        }
                }
                {
                        const Ray<N, T> r = ray.moved(-move_distance);
                        const auto t = box.intersect(r);
                        if (!t)
                        {
                                error("Ray must intersect, outside\nbox " + to_string(box) + "\nray " + to_string(r));
                        }
                        if (!(*t > move_min && *t < move_max))
                        {
                                error("Intersection out of bounding box, outside\ndistance = " + to_string(*t) + ", "
                                      + "min distance = " + to_string(move_min) + ", " + "max distance = "
                                      + to_string(move_max) + "\nbox " + to_string(box) + "\nray " + to_string(r));
                        }
                        const Vector<N, T> p = r.point(*t);
                        if (!is_on_box(box, p, precision))
                        {
                                error("Intersection out of bounding box, outside\nintersection point = " + to_string(p)
                                      + "\nbox " + to_string(box) + "\nray " + to_string(r));
                        }
                }
                {
                        const Ray<N, T> r = ray.moved(move_distance);
                        const auto t = box.intersect(r);
                        if (t)
                        {
                                error("Ray must not intersect\n" + to_string(r));
                        }
                }
        }
}

template <std::size_t N, typename T>
void test(const int point_count, const T& precision, std::mt19937_64& engine)
{
        test_intersections(create_random_box<N, T>(engine), point_count, precision, engine);
}

template <typename T>
void test(const int point_count, const T& precision, std::mt19937_64& engine)
{
        test<2, T>(point_count, precision, engine);
        test<3, T>(point_count, precision, engine);
        test<4, T>(point_count, precision, engine);
        test<5, T>(point_count, precision, engine);
}

void test_bounding_box()
{
        std::mt19937_64 engine = create_engine<std::mt19937_64>();

        LOG("Test bounding box");
        test<float>(10'000, 1e-5, engine);
        test<double>(10'000, 1e-14, engine);
        test<long double>(1'000, 1e-17, engine);
        LOG("Test bounding box passed");
}

TEST_SMALL("Bounding box", test_bounding_box)
}
}
