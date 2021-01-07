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

#include "test_normal.h"

#include "../complement.h"
#include "../normal.h"

#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/random/engine.h>
#include <src/com/type/limit.h>
#include <src/com/type/name.h>
#include <src/random/sphere_uniform.h>

#include <random>

namespace ns::numerical
{
namespace
{
template <std::size_t N, typename T>
void compare_normals(const Vector<N, T>& real_normal, const Vector<N, T>& computed_normal, T min_dot_product)
{
        const T d = std::abs(dot(real_normal, computed_normal));
        if (d < min_dot_product)
        {
                error(std::string("Random point normal error for ") + type_name<T>() + ": computed normal "
                      + to_string(computed_normal) + " is not equal to real normal " + to_string(real_normal)
                      + ", dot product " + to_string(d));
        }
}

template <std::size_t N, typename T>
void test_normal_defined()
{
        std::vector<Vector<N, T>> points;
        for (unsigned i = 0; i < N; ++i)
        {
                Vector<N, T>& p = points.emplace_back();
                for (unsigned j = 0; j < i; ++j)
                {
                        p[j] = 0;
                }
                p[i] = 1;
                for (unsigned j = i + 1; j < N; ++j)
                {
                        p[j] = 0;
                }
        }

        const Vector<N, T> computed_normal = point_normal(points);
        const Vector<N, T> real_normal = Vector<N, T>(1).normalized();

        compare_normals(real_normal, computed_normal, T(0.9999999));
}

template <std::size_t N, typename T, typename RandomEngine>
std::vector<Vector<N, T>> random_vectors(unsigned count, RandomEngine& random_engine)
{
        std::vector<Vector<N, T>> res;
        res.reserve(count);
        for (unsigned i = 0; i < count; ++i)
        {
                res.push_back(random::random_on_sphere<N, T>(random_engine));
        }
        return res;
}

template <std::size_t N, typename T>
void test_normal_random(unsigned test_count)
{
        constexpr unsigned POINT_COUNT = 100;

        std::mt19937_64 random_engine = create_engine<std::mt19937_64>();

        std::uniform_real_distribution<T> urd_for_normal(T(0.0), T(0.01));
        std::uniform_real_distribution<T> urd_for_complement(T(0.1), T(1.0));

        for (const Vector<N, T>& real_normal : random_vectors<N, T>(test_count, random_engine))
        {
                const std::array<Vector<N, T>, N - 1> vectors = orthogonal_complement_of_unit_vector(real_normal);

                std::vector<Vector<N, T>> points(POINT_COUNT);
                for (Vector<N, T>& p : points)
                {
                        p = urd_for_normal(random_engine) * real_normal;
                        for (const Vector<N, T>& v : vectors)
                        {
                                p += urd_for_complement(random_engine) * v;
                        }
                }

                const Vector<N, T> computed_normal = point_normal(points);

                compare_normals(real_normal, computed_normal, T(0.999));
        }
}

template <typename T>
void test_normal_defined()
{
        test_normal_defined<2, T>();
        test_normal_defined<3, T>();
        test_normal_defined<4, T>();
        test_normal_defined<5, T>();
        test_normal_defined<6, T>();
        test_normal_defined<7, T>();
        test_normal_defined<8, T>();
        test_normal_defined<9, T>();
        test_normal_defined<10, T>();
}

template <typename T>
void test_normal_random(unsigned test_count)
{
        test_normal_random<2, T>(test_count);
        test_normal_random<3, T>(test_count);
        test_normal_random<4, T>(test_count);
        test_normal_random<5, T>(test_count);
        test_normal_random<6, T>(test_count);
        test_normal_random<7, T>(test_count);
        test_normal_random<8, T>(test_count);
        test_normal_random<9, T>(test_count);
        test_normal_random<10, T>(test_count);
}
}

void test_normal(ProgressRatio* progress)
{
        LOG("Test point normals");
        progress->set(0);

        test_normal_defined<float>();
        test_normal_defined<double>();
        test_normal_defined<long double>();
        progress->set(1, 4);

        test_normal_random<float>(10);
        progress->set(2, 4);

        test_normal_random<double>(10);
        progress->set(3, 4);

        test_normal_random<long double>(10);
        progress->set(4, 4);

        LOG("Test point normals passed");
}
}
