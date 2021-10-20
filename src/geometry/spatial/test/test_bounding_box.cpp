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

#include <src/com/benchmark.h>
#include <src/com/chrono.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/random/engine.h>
#include <src/com/type/name.h>
#include <src/sampling/sphere_uniform.h>
#include <src/test/test.h>

#include <cmath>
#include <limits>
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
Vector<N, T> create_random_direction(const std::type_identity_t<T>& probability, std::mt19937_64& engine)
{
        if (std::bernoulli_distribution(probability)(engine))
        {
                return sampling::uniform_on_sphere<N, T>(engine);
        }
        const std::size_t n = std::uniform_int_distribution<std::size_t>(0, N - 1)(engine);
        Vector<N, T> v(0);
        v[n] = 1;
        return v;
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
        if (!is_on_box(box, p, precision))
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
        if (!is_on_box(box, p, precision))
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
void test_no_intersection(const BoundingBox<N, T>& box, const Ray<N, T>& ray, const std::optional<T>& t)
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
Vector<N, bool> non_negative(const Vector<N, T>& v)
{
        Vector<N, bool> res;
        for (std::size_t i = 0; i < N; ++i)
        {
                res[i] = (v[i] >= 0);
        }
        return res;
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
        const T random_direction_probability = 1 - T(1) / point_count;

        for (const Vector<N, T>& point : internal_points(box.min(), box_vectors(box), point_count, engine))
        {
                const Ray<N, T> ray(point, create_random_direction<N, T>(random_direction_probability, engine));

                {
                        const Ray<N, T> r = ray;
                        test_intersection_1(box, r, box.intersect(r), length, precision);
                        test_intersection_1(
                                box, r, box.intersect(r.org(), reciprocal(r.dir()), non_negative(r.dir())), length,
                                precision);
                }

                {
                        const Ray<N, T> r = ray.moved(-move_distance);
                        test_intersection_2(box, r, box.intersect(r), move_min, move_max, precision);
                        test_intersection_2(
                                box, r, box.intersect(r.org(), reciprocal(r.dir()), non_negative(r.dir())), move_min,
                                move_max, precision);
                }
                {
                        const Ray<N, T> r = ray.moved(move_distance);
                        test_no_intersection(box, r, box.intersect(r));
                        test_no_intersection(
                                box, r, box.intersect(r.org(), reciprocal(r.dir()), non_negative(r.dir())));
                }
        }
}

template <std::size_t N, typename T>
void test(const int point_count, const std::type_identity_t<T>& precision, std::mt19937_64& engine)
{
        const BoundingBox<N, T> box = create_random_box<N, T>(engine);
        test_intersections(box, point_count, precision, engine);
}

template <typename T>
void test(const int point_count, const std::type_identity_t<T>& precision, std::mt19937_64& engine)
{
        if (!(std::min<T>(1, std::numeric_limits<T>::quiet_NaN()) == 1))
        {
                error("std::min with NaN does not return the first argument");
        }
        if (!(std::max<T>(1, std::numeric_limits<T>::quiet_NaN()) == 1))
        {
                error("std::max with NaN does not return the first argument");
        }

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

//

template <std::size_t N, typename T>
std::vector<Ray<N, T>> rays_for_intersections(
        const BoundingBox<N, T>& box,
        const int point_count,
        std::mt19937_64& engine)
{
        const T length = box.diagonal().norm();
        const T move_distance = length;
        const int ray_count = 3 * point_count;
        std::vector<Ray<N, T>> rays;
        rays.reserve(ray_count);
        for (const Vector<N, T>& point : internal_points(box.min(), box_vectors(box), point_count, engine))
        {
                const Ray<N, T> ray(point, sampling::uniform_on_sphere<N, T>(engine));
                rays.push_back(ray);
                rays.push_back(ray.moved(-move_distance));
                rays.push_back(ray.moved(move_distance));
        }
        ASSERT(rays.size() == static_cast<std::size_t>(ray_count));
        return rays;
}

template <std::size_t N, typename T>
std::vector<Vector<N, T>> ray_orgs(const std::vector<Ray<N, T>>& rays)
{
        std::vector<Vector<N, T>> res;
        res.reserve(rays.size());
        for (const Ray<N, T>& ray : rays)
        {
                res.push_back(ray.org());
        }
        return res;
}

template <std::size_t N, typename T>
std::vector<Vector<N, T>> ray_reciprocal_directions(const std::vector<Ray<N, T>>& rays)
{
        std::vector<Vector<N, T>> res;
        res.reserve(rays.size());
        for (const Ray<N, T>& ray : rays)
        {
                res.push_back(reciprocal(ray.dir()));
        }
        return res;
}

template <std::size_t N, typename T>
std::vector<Vector<N, bool>> ray_non_negative_directions(const std::vector<Ray<N, T>>& rays)
{
        std::vector<Vector<N, bool>> res;
        res.reserve(rays.size());
        for (const Ray<N, T>& ray : rays)
        {
                res.push_back(non_negative(ray.dir()));
        }
        return res;
}

template <std::size_t N, typename T>
double compute_intersections_per_second(const int point_count, std::mt19937_64& engine)
{
        const BoundingBox<N, T> box = create_random_box<N, T>(engine);
        const std::vector<Ray<N, T>> rays = rays_for_intersections(box, point_count, engine);
        const auto f = [&]
        {
                for (const Ray<N, T>& ray : rays)
                {
                        do_not_optimize(box.intersect(ray));
                }
        };
        f();
        Clock::time_point start_time = Clock::now();
        f();
        return rays.size() / duration_from(start_time);
}

template <std::size_t N, typename T>
double compute_intersections_r_per_second(const int point_count, std::mt19937_64& engine)
{
        const BoundingBox<N, T> box = create_random_box<N, T>(engine);
        const std::vector<Ray<N, T>> rays = rays_for_intersections(box, point_count, engine);
        const std::vector<Vector<N, T>> orgs = ray_orgs(rays);
        const std::vector<Vector<N, T>> r_dirs = ray_reciprocal_directions(rays);
        const std::vector<Vector<N, bool>> non_negative = ray_non_negative_directions(rays);
        const std::size_t n = rays.size();
        const auto f = [&]
        {
                for (std::size_t i = 0; i < n; ++i)
                {
                        do_not_optimize(box.intersect(orgs[i], r_dirs[i], non_negative[i]));
                }
        };
        f();
        Clock::time_point start_time = Clock::now();
        f();
        return rays.size() / duration_from(start_time);
}

template <typename F>
double average(const int count, const F& f)
{
        double sum = 0;
        for (int i = 0; i < count; ++i)
        {
                sum += f();
        }
        return sum / count;
}

template <std::size_t N, typename T>
double compute_intersections_per_second()
{
        constexpr int AVERAGE_COUNT = 100;
        constexpr int POINT_COUNT = 100'000;
        std::mt19937_64 engine = create_engine<std::mt19937_64>();
        return average(
                AVERAGE_COUNT,
                [&]
                {
                        return compute_intersections_per_second<N, T>(POINT_COUNT, engine);
                });
}

template <std::size_t N, typename T>
double compute_intersections_r_per_second()
{
        constexpr int AVERAGE_COUNT = 100;
        constexpr int POINT_COUNT = 100'000;
        std::mt19937_64 engine = create_engine<std::mt19937_64>();
        return average(
                AVERAGE_COUNT,
                [&]
                {
                        return compute_intersections_r_per_second<N, T>(POINT_COUNT, engine);
                });
}

template <std::size_t N, typename T>
void test_performance()
{
        const long long p_1 = std::lround(compute_intersections_per_second<N, T>());
        LOG("<" + to_string(N) + ", " + type_name<T>() + ">, #1 = " + to_string_digit_groups(p_1)
            + " intersections per second");

        const long long p_2 = std::lround(compute_intersections_r_per_second<N, T>());
        LOG("<" + to_string(N) + ", " + type_name<T>() + ">, #2 = " + to_string_digit_groups(p_2)
            + " intersections per second");
}

template <typename T>
void test_performance()
{
        test_performance<2, T>();
        LOG("");
        test_performance<3, T>();
        LOG("");
        test_performance<4, T>();
        LOG("");
        test_performance<5, T>();
}

void test_bounding_box_performance()
{
        test_performance<float>();
        LOG("");
        test_performance<double>();
}

//

TEST_SMALL("Bounding box", test_bounding_box)
TEST_PERFORMANCE("Bounding box", test_bounding_box_performance)
}
}
