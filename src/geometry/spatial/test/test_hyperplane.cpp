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

#include "../hyperplane.h"

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/random/pcg.h>
#include <src/test/test.h>

#include <random>
#include <sstream>

namespace ns::geometry::spatial
{
namespace
{
template <std::size_t N, typename T, typename RandomEngine>
Vector<N, T> random_vector(RandomEngine& engine)
{
        std::uniform_real_distribution<T> urd(-5, 5);
        Vector<N, T> v;
        for (std::size_t i = 0; i < N; ++i)
        {
                v[i] = urd(engine);
        }
        return v;
}

template <std::size_t N, typename T, typename RandomEngine>
Vector<N, T> random_direction(RandomEngine& engine)
{
        std::uniform_real_distribution<T> urd(-5, 5);
        Vector<N, T> v;
        do
        {
                for (std::size_t i = 0; i < N; ++i)
                {
                        v[i] = urd(engine);
                }
                v.normalize();
        } while (!is_finite(v));
        return v;
}

template <std::size_t N, typename T, typename RandomEngine>
Ray<N, T> random_ray(RandomEngine& engine)
{
        Ray<N, T> ray;
        do
        {
                ray.set_org(random_vector<N, T>(engine));
                ray.set_dir(random_vector<N, T>(engine));
        } while (!is_finite(ray.dir()));
        return ray;
}

template <std::size_t N, typename T>
bool test_point_on_plane(
        const T& precision,
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

template <std::size_t N, typename T, typename RandomEngine>
void test_intersect(const T& precision, RandomEngine& engine)
{
        constexpr int COUNT = 100;

        const Vector<N, T> plane_normal = random_direction<N, T>(engine);
        const Vector<N, T> plane_point = random_vector<N, T>(engine);
        const Hyperplane plane(plane_normal, dot(plane_normal, plane_point));

        int i_sum = 0;
        int m_sum = 0;
        for (int i = 0; i < COUNT; ++i)
        {
                const Ray<N, T> ray = random_ray<N, T>(engine);
                const T t = plane.intersect(ray);
                if (!(t > 0))
                {
                        ++m_sum;
                        continue;
                }
                if (test_point_on_plane(precision, ray.point(t), plane, plane_point))
                {
                        ++i_sum;
                }
        }

        if (!(i_sum >= COUNT * 0.2 && m_sum >= COUNT * 0.2))
        {
                error("Error intersect, ray count = " + to_string(COUNT) + ", intersections = " + to_string(i_sum)
                      + ", missed = " + to_string(m_sum));
        }
}

template <std::size_t N, typename T, typename RandomEngine>
void test_project(const T& precision, RandomEngine& engine)
{
        constexpr int COUNT = 100;

        const Vector<N, T> plane_normal = random_direction<N, T>(engine);
        const Vector<N, T> plane_point = random_vector<N, T>(engine);
        const Hyperplane plane(plane_normal, dot(plane_normal, plane_point));

        int sum = 0;
        for (int i = 0; i < COUNT; ++i)
        {
                const Vector<N, T> point = random_vector<N, T>(engine);
                const Vector<N, T> projection = plane.project(point);
                if (test_point_on_plane(precision, projection, plane, plane_point))
                {
                        ++sum;
                }
        }

        if (!(sum >= COUNT * 0.8))
        {
                error("Error project, point count = " + to_string(COUNT) + ", projections = " + to_string(sum));
        }
}

template <std::size_t N, typename T, typename RandomEngine>
void test(const T& precision, RandomEngine& engine)
{
        test_intersect<N, T>(precision, engine);
        test_project<N, T>(precision, engine);
}

template <typename T, typename RandomEngine>
void test(const T& precision, RandomEngine& engine)
{
        test<2, T>(precision, engine);
        test<3, T>(precision, engine);
        test<4, T>(precision, engine);
        test<5, T>(precision, engine);
}

void test_hyperplane()
{
        PCG engine;

        LOG("Test hyperplane");
        test<float>(1e-4, engine);
        test<double>(1e-13, engine);
        test<long double>(1e-16, engine);
        LOG("Test hyperplane passed");
}

TEST_SMALL("Hyperplane", test_hyperplane)
}
}
