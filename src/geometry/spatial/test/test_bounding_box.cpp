/*
Copyright (C) 2017-2023 Topological Manifold

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

#include "../bounding_box.h"
#include "../random/parallelotope_points.h"

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/random/pcg.h>
#include <src/sampling/sphere_uniform.h>
#include <src/test/test.h>

#include <cmath>
#include <random>

namespace ns::geometry::spatial::test
{
namespace
{
template <typename T>
struct Test final
{
        static constexpr BoundingBox<4, T> BOX{Vector<4, T>(1, -2, 3, -4), Vector<4, T>(-5, 6, -7, 8)};
        static_assert(BOX.min() == Vector<4, T>(-5, -2, -7, -4));
        static_assert(BOX.max() == Vector<4, T>(1, 6, 3, 8));
        static_assert(BOX.diagonal() == Vector<4, T>(6, 8, 10, 12));
        static_assert(BOX.center() == Vector<4, T>(-2, 2, -2, 2));
        static_assert(BOX.volume() == 5760);
        static_assert(BOX.surface() == 5472);
        static_assert(BOX.maximum_extent() == 3);

        static constexpr Vector<4, T> MERGE_POINT{Vector<4, T>(5, -5, 5, -5)};
        static constexpr BoundingBox<4, T> BOX_MERGE_POINT = []
        {
                BoundingBox<4, T> b(BOX);
                b.merge(MERGE_POINT);
                return b;
        }();
        static_assert(BOX_MERGE_POINT.min() == Vector<4, T>(-5, -5, -7, -5));
        static_assert(BOX_MERGE_POINT.max() == Vector<4, T>(5, 6, 5, 8));
        static_assert(BOX.merged(MERGE_POINT).min() == BOX_MERGE_POINT.min());
        static_assert(BOX.merged(MERGE_POINT).max() == BOX_MERGE_POINT.max());

        static constexpr BoundingBox<4, T> MERGE_BOX{
                BoundingBox<4, T>(Vector<4, T>(4, -3, 2, -1), Vector<4, T>(-4, 5, -6, 7))};
        static constexpr BoundingBox<4, T> BOX_MERGE_BOX = []
        {
                BoundingBox<4, T> b(BOX);
                b.merge(MERGE_BOX);
                return b;
        }();
        static_assert(BOX_MERGE_BOX.min() == Vector<4, T>(-5, -3, -7, -4));
        static_assert(BOX_MERGE_BOX.max() == Vector<4, T>(4, 6, 3, 8));
        static_assert(BOX.merged(MERGE_BOX).min() == BOX_MERGE_BOX.min());
        static_assert(BOX.merged(MERGE_BOX).max() == BOX_MERGE_BOX.max());

        static constexpr BoundingBox<4, T> BOX_POINT{Vector<4, T>(1, -2, 3, -4)};
        static_assert(BOX_POINT.min() == Vector<4, T>(1, -2, 3, -4));
        static_assert(BOX_POINT.max() == Vector<4, T>(1, -2, 3, -4));

        static constexpr BoundingBox<4, T> BOX_ARRAY{
                std::array{Vector<4, T>(1, -2, 3, -4), Vector<4, T>(-5, 6, -7, 8)}
        };
        static_assert(BOX_ARRAY.min() == Vector<4, T>(-5, -2, -7, -4));
        static_assert(BOX_ARRAY.max() == Vector<4, T>(1, 6, 3, 8));
};

template struct Test<float>;
template struct Test<double>;
template struct Test<long double>;

//

template <std::size_t N, typename T, typename RandomEngine>
BoundingBox<N, T> create_random_bounding_box(RandomEngine& engine)
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
                } while (!(std::abs(p1[i] - p2[i]) >= T{0.5}));
        }
        return {p1, p2};
}

template <std::size_t N, typename T, typename RandomEngine>
Vector<N, T> create_random_direction(const std::type_identity_t<T>& probability, RandomEngine& engine)
{
        if (std::bernoulli_distribution(probability)(engine))
        {
                return sampling::uniform_on_sphere<N, T>(engine);
        }
        const std::size_t n = std::uniform_int_distribution<std::size_t>(0, N - 1)(engine);
        Vector<N, T> v(0);
        v[n] = 1;
        return std::bernoulli_distribution(0.5)(engine) ? v : -v;
}

template <std::size_t N, typename T, typename RandomEngine>
Vector<N, T> create_random_aa_direction(RandomEngine& engine)
{
        const std::size_t n = std::uniform_int_distribution<std::size_t>(0, N - 1)(engine);
        Vector<N, T> v(1);
        v[n] = 0;
        return std::bernoulli_distribution(0.5)(engine) ? v : -v;
}

template <std::size_t N, typename T>
bool test_on_box(const BoundingBox<N, T>& box, const Vector<N, T>& point, const T& precision)
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
void test_intersection_1(
        const BoundingBox<N, T>& box,
        const Ray<N, T>& ray,
        const std::optional<T>& t,
        const T& length,
        const T& precision)
{
        if (!t)
        {
                std::string s;
                s += "Ray must intersect, inside\n";
                s += "box " + to_string(box) + "\n";
                s += "ray " + to_string(ray);
                error(s);
        }

        if (!(*t > 0 && *t < length))
        {
                std::string s;
                s += "Intersection out of bounding box, inside\n";
                s += "distance = " + to_string(*t) + ", max distance = " + to_string(length) + "\n";
                s += "box " + to_string(box) + "\n";
                s += "ray " + to_string(ray);
                error(s);
        }

        const Vector<N, T> p = ray.point(*t);
        if (!test_on_box(box, p, precision))
        {
                std::string s;
                s += "Intersection out of bounding box, inside\n";
                s += "intersection point = " + to_string(p) + "\n";
                s += "box " + to_string(box) + "\n";
                s += "ray " + to_string(ray);
                error(s);
        }
}

template <std::size_t N, typename T>
void test_intersection_1_volume(
        const BoundingBox<N, T>& box,
        const Ray<N, T>& ray,
        const std::optional<T>& t,
        const T& length)
{
        if (!t)
        {
                std::string s;
                s += "Ray must intersect, inside\n";
                s += "box " + to_string(box) + "\n";
                s += "ray " + to_string(ray);
                error(s);
        }

        if (!(*t == 0))
        {
                std::string s;
                s += "Intersection out of bounding box, inside\n";
                s += "distance = " + to_string(*t) + ", max distance = " + to_string(length) + "\n";
                s += "box " + to_string(box) + "\n";
                s += "ray " + to_string(ray);
                error(s);
        }
}

template <std::size_t N, typename T>
void test_intersection_1(const BoundingBox<N, T>& box, const Ray<N, T>& ray, const bool t)
{
        if (t)
        {
                return;
        }

        std::string s;
        s += "Ray must intersect, inside\n";
        s += "box " + to_string(box) + "\n";
        s += "ray " + to_string(ray);
        error(s);
}

template <std::size_t N, typename T>
void test_intersection_2(
        const BoundingBox<N, T>& box,
        const Ray<N, T>& ray,
        const std::optional<T>& t,
        const T& move_min,
        const T& move_max,
        const T& precision)
{
        if (!t)
        {
                std::string s;
                s += "Ray must intersect, outside\n";
                s += "box " + to_string(box) + "\n";
                s += "ray " + to_string(ray);
                error(s);
        }

        if (!(*t > move_min && *t < move_max))
        {
                std::string s;
                s += "Intersection out of bounding box, outside\n";
                s += "distance = " + to_string(*t) + ", " + "min distance = " + to_string(move_min)
                     + ", max distance = " + to_string(move_max) + "\n";
                s += "box " + to_string(box) + "\n";
                s += "ray " + to_string(ray);
                error(s);
        }

        const Vector<N, T> p = ray.point(*t);
        if (!test_on_box(box, p, precision))
        {
                std::string s;
                s += "Intersection out of bounding box, outside\n";
                s += "intersection point = " + to_string(p) + "\n";
                s += "box " + to_string(box) + "\n";
                s += "ray " + to_string(ray);
                error(s);
        }
}

template <std::size_t N, typename T>
void test_intersection_2(const BoundingBox<N, T>& box, const Ray<N, T>& ray, const bool t)
{
        if (t)
        {
                return;
        }

        std::string s;
        s += "Ray must intersect, outside\n";
        s += "box " + to_string(box) + "\n";
        s += "ray " + to_string(ray);
        error(s);
}

template <std::size_t N, typename T, typename P>
void test_no_intersection(const BoundingBox<N, T>& box, const Ray<N, T>& ray, const P& t)
{
        if (!t)
        {
                return;
        }

        std::string s;
        s += "Ray must not intersect\n";
        s += "box " + to_string(box) + "\n";
        s += "ray " + to_string(ray);
        error(s);
}

template <std::size_t N, typename T>
void test_inside(
        const BoundingBox<N, T>& box,
        const T precision,
        const Ray<N, T>& ray,
        const T length,
        const T move_distance)
{
        const Ray<N, T> r = ray;

        test_intersection_1(box, r, box.intersect(r), length, precision);
        test_intersection_1(box, r, box.intersect(r, move_distance), length, precision);

        test_intersection_1_volume(box, r, box.intersect_volume(r), length);
        test_intersection_1_volume(box, r, box.intersect_volume(r, move_distance), length);

        test_intersection_1(box, r, box.intersect(r.org(), reciprocal(r.dir()), negative_bool(r.dir())));
        test_intersection_1(box, r, box.intersect(r.org(), reciprocal(r.dir()), negative_bool(r.dir()), move_distance));
}

template <std::size_t N, typename T>
void test_outside(
        const BoundingBox<N, T>& box,
        const T precision,
        const Ray<N, T>& ray,
        const T length,
        const T move_distance,
        const T move_min,
        const T move_max)
{
        const Ray<N, T> r = ray.moved(-move_distance);

        test_intersection_2(box, r, box.intersect(r), move_min, move_max, precision);
        test_intersection_2(box, r, box.intersect(r, move_distance), move_min, move_max, precision);
        test_no_intersection(box, r, box.intersect(r, length));

        test_intersection_2(box, r, box.intersect_volume(r), move_min, move_max, precision);
        test_intersection_2(box, r, box.intersect_volume(r, move_distance), move_min, move_max, precision);
        test_no_intersection(box, r, box.intersect_volume(r, length));

        test_intersection_2(box, r, box.intersect(r.org(), reciprocal(r.dir()), negative_bool(r.dir())));
        test_intersection_2(box, r, box.intersect(r.org(), reciprocal(r.dir()), negative_bool(r.dir()), move_distance));
        test_no_intersection(box, r, box.intersect(r.org(), reciprocal(r.dir()), negative_bool(r.dir()), length));
}

template <std::size_t N, typename T>
void test_outside_no_intersection(
        const BoundingBox<N, T>& box,
        const Ray<N, T>& ray,
        const T length,
        const T move_distance)
{
        const Ray<N, T> r = ray.moved(move_distance);

        test_no_intersection(box, r, box.intersect(r));
        test_no_intersection(box, r, box.intersect(r, length));

        test_no_intersection(box, r, box.intersect_volume(r));
        test_no_intersection(box, r, box.intersect_volume(r, length));

        test_no_intersection(box, r, box.intersect(r.org(), reciprocal(r.dir()), negative_bool(r.dir())));
        test_no_intersection(box, r, box.intersect(r.org(), reciprocal(r.dir()), negative_bool(r.dir()), length));
}

template <std::size_t N, typename T, typename RandomEngine>
void test_intersections(const BoundingBox<N, T>& box, const int point_count, const T precision, RandomEngine& engine)
{
        const T length = box.diagonal().norm();
        const T move_distance = 2 * length;
        const T move_min = length;
        const T move_max = 2 * length;
        const T random_direction_probability = 0.99;

        for (const Vector<N, T>& point :
             random::parallelotope_internal_points(box.min(), box.diagonal(), point_count, engine))
        {
                const Ray<N, T> ray(point, create_random_direction<N, T>(random_direction_probability, engine));

                test_inside(box, precision, ray, length, move_distance);

                test_outside(box, precision, ray, length, move_distance, move_min, move_max);

                test_outside_no_intersection(box, ray, length, move_distance);
        }
}

template <std::size_t N, typename T, typename RandomEngine>
void test_external(const BoundingBox<N, T>& box, const int point_count, RandomEngine& engine)
{
        for (const Vector<N, T>& point :
             random::parallelotope_external_points(box.min(), box.diagonal(), point_count, engine))
        {
                const Ray<N, T> ray(point, create_random_aa_direction<N, T>(engine));

                test_no_intersection(box, ray, box.intersect(ray));

                test_no_intersection(box, ray, box.intersect_volume(ray));

                test_no_intersection(
                        box, ray, box.intersect(ray.org(), reciprocal(ray.dir()), negative_bool(ray.dir())));
        }
}

template <std::size_t N, typename T, typename RandomEngine>
void test(const int point_count, const std::type_identity_t<T>& precision, RandomEngine& engine)
{
        const BoundingBox<N, T> box = create_random_bounding_box<N, T>(engine);
        test_intersections(box, point_count, precision, engine);
        test_external(box, std::max(1, point_count / 10), engine);
}

template <typename T, typename RandomEngine>
void test(const int point_count, const std::type_identity_t<T>& precision, RandomEngine& engine)
{
        test<2, T>(point_count, precision, engine);
        test<3, T>(point_count, precision, engine);
        test<4, T>(point_count, precision, engine);
        test<5, T>(point_count, precision, engine);
}

void test_bounding_box()
{
        PCG engine;

        LOG("Test bounding box");
        test<float>(10'000, 1e-5, engine);
        test<double>(10'000, 1e-14, engine);
        test<long double>(1'000, 1e-17, engine);
        LOG("Test bounding box passed");
}

TEST_SMALL("Bounding Box", test_bounding_box)
}
}
