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

#include "../ggx.h"
#include "distribution/distribution.h"

#include <src/com/log.h>
#include <src/com/names.h>
#include <src/com/print.h>
#include <src/com/random/engine.h>
#include <src/com/type/name.h>
#include <src/test/test.h>

#include <cmath>
#include <random>

namespace ns::sampling::test
{
namespace
{
constexpr long long UNIT_COUNT = 10'000'000;
constexpr long long ANGLE_COUNT_PER_BUCKET = 1'000;
constexpr long long SURFACE_COUNT_PER_BUCKET = 10'000;
constexpr long long PERFORMANCE_COUNT = 10'000'000;

template <typename T>
using RandomEngine = std::conditional_t<sizeof(T) <= 4, std::mt19937, std::mt19937_64>;

template <std::size_t N, typename T>
void test_ggx()
{
        const T alpha = []()
        {
                RandomEngine<T> random_engine = create_engine<RandomEngine<T>>();
                return std::uniform_real_distribution<T>(0.1, 1)(random_engine);
        }();

        LOG("GGX, " + space_name(N) + ", " + type_name<T>() + ", alpha " + to_string_fixed(alpha, 2));

        const Vector<N, T> normal = []()
        {
                RandomEngine<T> random_engine = create_engine<RandomEngine<T>>();
                return uniform_on_sphere<N, T>(random_engine).normalized();
        }();

        test_unit<N, T, RandomEngine<T>>(
                "Visible Normals", UNIT_COUNT,
                [&](RandomEngine<T>& random_engine)
                {
                        Vector<N, T> v = uniform_on_sphere<N, T>(random_engine);
                        if (dot(v, normal) < 0)
                        {
                                v = -v;
                        }
                        return ggx_visible_normals_h(random_engine, normal, v, alpha);
                });

        test_unit<N, T, RandomEngine<T>>(
                "Visible Normals, Reflected", UNIT_COUNT,
                [&](RandomEngine<T>& random_engine)
                {
                        Vector<N, T> v = uniform_on_sphere<N, T>(random_engine);
                        if (dot(v, normal) < 0)
                        {
                                v = -v;
                        }
                        const auto [h, l] = ggx_visible_normals_h_l(random_engine, normal, v, alpha);
                        return l;
                });

        test_distribution_angle<N, T, RandomEngine<T>>(
                "Normals", ANGLE_COUNT_PER_BUCKET, normal,
                [&](RandomEngine<T>& random_engine)
                {
                        return ggx_visible_normals_h(random_engine, normal, normal, alpha);
                },
                [&](T angle)
                {
                        const T n_h = std::cos(angle);
                        return n_h * ggx_pdf<N>(n_h, alpha);
                });

        test_distribution_surface<N, T, RandomEngine<T>>(
                "Normals", SURFACE_COUNT_PER_BUCKET,
                [&](RandomEngine<T>& random_engine)
                {
                        return ggx_visible_normals_h(random_engine, normal, normal, alpha);
                },
                [&](const Vector<N, T>& v)
                {
                        const T n_h = dot(normal, v);
                        return n_h * ggx_pdf<N>(n_h, alpha);
                });

        const Vector<N, T> v = [&]()
        {
                RandomEngine<T> random_engine = create_engine<RandomEngine<T>>();
                Vector<N, T> r = uniform_on_sphere<N, T>(random_engine).normalized();
                if (dot(r, normal) < 0)
                {
                        return -r;
                }
                return r;
        }();

        const T n_v = dot(normal, v);

        test_distribution_surface<N, T, RandomEngine<T>>(
                "Visible Normals", SURFACE_COUNT_PER_BUCKET,
                [&](RandomEngine<T>& random_engine)
                {
                        return ggx_visible_normals_h(random_engine, normal, v, alpha);
                },
                [&](const Vector<N, T>& h)
                {
                        const T n_h = dot(normal, h);
                        const T h_v = dot(h, v);
                        return ggx_visible_normals_h_pdf<N>(n_v, n_h, h_v, alpha);
                });

        test_distribution_surface<N, T, RandomEngine<T>>(
                "Visible Normals, Reflected", SURFACE_COUNT_PER_BUCKET,
                [&](RandomEngine<T>& random_engine)
                {
                        const auto [h, l] = ggx_visible_normals_h_l(random_engine, normal, v, alpha);
                        return l;
                },
                [&](const Vector<N, T>& l)
                {
                        const Vector<N, T> h = (l + v).normalized();
                        const T n_h = dot(normal, h);
                        const T h_v = dot(h, v);
                        return ggx_visible_normals_l_pdf<N>(n_v, n_h, h_v, alpha);
                });

        test_performance<N, T, RandomEngine<T>>(
                "Visible Normals", PERFORMANCE_COUNT,
                [&](RandomEngine<T>& random_engine)
                {
                        return ggx_visible_normals_h(random_engine, normal, v, alpha);
                });

        test_performance<N, T, RandomEngine<T>>(
                "Visible Normals, Reflected", PERFORMANCE_COUNT,
                [&](RandomEngine<T>& random_engine)
                {
                        const auto [h, l] = ggx_visible_normals_h_l(random_engine, normal, v, alpha);
                        return l;
                });
}

template <std::size_t N>
void test_ggx()
{
        test_ggx<N, float>();
        test_ggx<N, double>();
}

TEST_LARGE("Sample Distribution, GGX, 3-Space", test_ggx<3>)
TEST_LARGE("Sample Distribution, GGX, 4-Space", test_ggx<4>)
TEST_LARGE("Sample Distribution, GGX, 5-Space", test_ggx<5>)
}
}
