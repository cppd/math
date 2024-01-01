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

#include "../hyperplane.h"
#include "../random/vectors.h"

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/random/pcg.h>
#include <src/numerical/complement.h>
#include <src/numerical/ray.h>
#include <src/numerical/vector.h>
#include <src/sampling/sphere_uniform.h>
#include <src/test/test.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <random>
#include <sstream>

namespace ns::geometry::spatial::test
{
namespace
{
template <typename T>
constexpr T INTERVAL = 5;

template <std::size_t N, typename T>
constexpr Hyperplane<N, T> reverse(Hyperplane<N, T> plane)
{
        plane.reverse_normal();
        return plane;
}

template <typename T>
struct Test final
{
        static_assert(Hyperplane<4, T>({1.1, 1.2, 1.3, 1.4}, 1.5).n == Vector<4, T>(1.1, 1.2, 1.3, 1.4));
        static_assert(Hyperplane<4, T>({1.1, 1.2, 1.3, 1.4}, 1.5).d == 1.5);
        static_assert(Hyperplane<4, T>({1.1, 1.2, 1.3, 1.4, 1.5}).n == Vector<4, T>(1.1, 1.2, 1.3, 1.4));
        static_assert(Hyperplane<4, T>({1.1, 1.2, 1.3, 1.4, 1.5}).d == -1.5);

        static_assert(reverse(Hyperplane<4, T>({1.1, 1.2, 1.3, 1.4}, 1.5)).n == -Vector<4, T>(1.1, 1.2, 1.3, 1.4));
        static_assert(reverse(Hyperplane<4, T>({1.1, 1.2, 1.3, 1.4}, 1.5)).d == -1.5);
};

template struct Test<float>;
template struct Test<double>;
template struct Test<long double>;

template <std::size_t N, typename T, typename RandomEngine>
Vector<N, T> random_plane_vector(
        const std::array<Vector<N, T>, N - 1>& plane_vectors,
        std::uniform_real_distribution<T>& urd,
        RandomEngine& engine)
{
        static_assert(N >= 2);
        Vector<N, T> res = urd(engine) * plane_vectors[0];
        for (std::size_t i = 1; i < plane_vectors.size(); ++i)
        {
                res.multiply_add(urd(engine), plane_vectors[i]);
        }
        return res;
}

template <std::size_t N, typename T>
bool test_point_on_plane(
        const T precision,
        const Vector<N, T>& point,
        const Hyperplane<N, T>& plane,
        const Vector<N, T>& plane_point)
{
        const Vector<N, T> to_point = point - plane_point;

        if (!(to_point.norm() > T{0.1}))
        {
                return false;
        }

        const T cosine = std::abs(dot(plane.n, to_point.normalized()));
        if (cosine < precision)
        {
                return true;
        }

        std::ostringstream oss;
        oss << "Point " << to_string(point) << " is not on the plane\n";
        oss << "n = " << to_string(plane.n) << "; d = " << to_string(plane.d) << "; p = " << to_string(plane_point)
            << "\n";
        oss << "distance = " << to_point.norm() << "; cosine = " << to_string(cosine);
        error(oss.str());
}

template <std::size_t N, typename T>
void test_point_distance(
        const T precision,
        const T distance,
        const T expected_distance,
        const Vector<N, T>& point,
        const Hyperplane<N, T>& plane,
        const Vector<N, T>& plane_point)
{
        if (std::abs(distance - expected_distance) < precision)
        {
                return;
        }

        std::ostringstream oss;
        oss << "Point distance error\n";
        oss << "Distance = " << distance << "; expected distance = " << to_string(expected_distance) << "\n";
        oss << "Point " << to_string(point) << "\n";
        oss << "n = " << to_string(plane.n) << "; d = " << to_string(plane.d) << "; p = " << to_string(plane_point);
        error(oss.str());
}

template <std::size_t N, typename T, typename RandomEngine>
void test_intersect(const T precision, RandomEngine& engine)
{
        constexpr int TEST_COUNT = 100;

        const Vector<N, T> plane_point = random::point<N, T>(INTERVAL<T>, engine);
        const Vector<N, T> plane_normal = sampling::uniform_on_sphere<N, T>(engine);
        const Hyperplane plane(plane_normal, dot(plane_normal, plane_point));

        int intersected_count = 0;
        int missed_count = 0;

        for (int i = 0; i < TEST_COUNT; ++i)
        {
                const Ray<N, T> random_ray(
                        random::point<N, T>(INTERVAL<T>, engine), sampling::uniform_on_sphere<N, T>(engine));

                const T t = plane.intersect(random_ray);

                if (!(t > 0))
                {
                        ++missed_count;
                        continue;
                }

                if (test_point_on_plane(precision, random_ray.point(t), plane, plane_point))
                {
                        ++intersected_count;
                }
        }

        if (!(intersected_count >= TEST_COUNT * 0.2 && missed_count >= TEST_COUNT * 0.2))
        {
                error("Error intersect, ray count = " + to_string(TEST_COUNT)
                      + ", intersections = " + to_string(intersected_count) + ", missed = " + to_string(missed_count));
        }
}

template <std::size_t N, typename T, typename RandomEngine>
void test_distance(const T precision, RandomEngine& engine)
{
        static_assert(N >= 2);

        constexpr int TEST_COUNT = 100;

        const Vector<N, T> plane_point = random::point<N, T>(INTERVAL<T>, engine);
        const Vector<N, T> plane_normal = sampling::uniform_on_sphere<N, T>(engine);
        const Hyperplane plane(plane_normal, dot(plane_normal, plane_point));

        const std::array<Vector<N, T>, N - 1> plane_vectors =
                numerical::orthogonal_complement_of_unit_vector(plane_normal.normalized());

        std::uniform_real_distribution<T> urd(-INTERVAL<T>, INTERVAL<T>);

        for (int i = 0; i < TEST_COUNT; ++i)
        {
                const T random_distance = urd(engine);

                const Vector<N, T> random_point =
                        plane_point + random_plane_vector(plane_vectors, urd, engine) + random_distance * plane_normal;

                test_point_distance(
                        precision, plane.distance(random_point), random_distance, random_point, plane, plane_point);
        }
}

template <std::size_t N, typename T, typename RandomEngine>
void test_project(const T precision, RandomEngine& engine)
{
        constexpr int TEST_COUNT = 100;

        const Vector<N, T> plane_point = random::point<N, T>(INTERVAL<T>, engine);
        const Vector<N, T> plane_normal = sampling::uniform_on_sphere<N, T>(engine);
        const Hyperplane plane(plane_normal, dot(plane_normal, plane_point));

        int projected_count = 0;

        for (int i = 0; i < TEST_COUNT; ++i)
        {
                const Vector<N, T> random_point = random::point<N, T>(INTERVAL<T>, engine);
                const Vector<N, T> projection = plane.project(random_point);

                if (test_point_on_plane(precision, projection, plane, plane_point))
                {
                        ++projected_count;
                }
        }

        if (!(projected_count >= TEST_COUNT * 0.8))
        {
                error("Error project, point count = " + to_string(TEST_COUNT)
                      + ", projections = " + to_string(projected_count));
        }
}

template <std::size_t N, typename T, typename RandomEngine>
void test(const T precision, const T distance_precision, RandomEngine& engine)
{
        test_intersect<N, T>(precision, engine);
        test_distance<N, T>(distance_precision, engine);
        test_project<N, T>(precision, engine);
}

template <typename T, typename RandomEngine>
void test(const T precision, const T distance_precision, RandomEngine& engine)
{
        test<2, T>(precision, distance_precision, engine);
        test<3, T>(precision, distance_precision, engine);
        test<4, T>(precision, distance_precision, engine);
        test<5, T>(precision, distance_precision, engine);
}

void test_hyperplane()
{
        PCG engine;

        LOG("Test hyperplane");
        test<float>(1e-4, 1e-5, engine);
        test<double>(1e-13, 1e-14, engine);
        test<long double>(1e-16, 1e-17, engine);
        LOG("Test hyperplane passed");
}

TEST_SMALL("Hyperplane", test_hyperplane)
}
}
