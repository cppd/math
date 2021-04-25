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
#include "../sphere_cosine.h"
#include "../sphere_uniform.h"
#include "distribution/distribution.h"

#include <src/com/log.h>
#include <src/com/random/engine.h>
#include <src/numerical/optics.h>
#include <src/test/test.h>

#include <cmath>
#include <random>
#include <string>

namespace ns::sampling::test
{
namespace
{
constexpr long long UNIT_COUNT = 10'000'000;
constexpr long long ANGLE_COUNT_PER_BUCKET = 1'000;
constexpr long long SURFACE_COUNT_PER_BUCKET = 10'000;
constexpr long long PERFORMANCE_COUNT = 10'000'000;

template <std::size_t N, typename T>
void test_uniform_on_sphere()
{
        const std::string name = "Uniform";

        const Vector<N, T> normal = []()
        {
                RandomEngine<T> random_engine = create_engine<RandomEngine<T>>();
                return uniform_on_sphere<N, T>(random_engine).normalized();
        }();

        test_unit<N, T>(
                name, UNIT_COUNT,
                [&](RandomEngine<T>& random_engine)
                {
                        return uniform_on_sphere<N, T>(random_engine);
                });

        test_distribution_angle<N, T>(
                name, ANGLE_COUNT_PER_BUCKET, normal,
                [&](RandomEngine<T>& random_engine)
                {
                        return uniform_on_sphere<N, T>(random_engine);
                },
                [](T /*angle*/)
                {
                        return uniform_on_sphere_pdf<N, T>();
                });

        test_distribution_surface<N, T>(
                name, SURFACE_COUNT_PER_BUCKET,
                [&](RandomEngine<T>& random_engine)
                {
                        return uniform_on_sphere<N, T>(random_engine);
                },
                [&](const Vector<N, T>& /*v*/)
                {
                        return uniform_on_sphere_pdf<N, T>();
                });

        test_performance<N, T>(
                name, PERFORMANCE_COUNT,
                [&](RandomEngine<T>& random_engine)
                {
                        return uniform_on_sphere<N, T>(random_engine);
                });
}

template <std::size_t N, typename T>
void test_cosine_on_hemisphere()
{
        const std::string name = "Cosine";

        const Vector<N, T> normal = []()
        {
                RandomEngine<T> random_engine = create_engine<RandomEngine<T>>();
                return uniform_on_sphere<N, T>(random_engine).normalized();
        }();

        test_unit<N, T>(
                name, UNIT_COUNT,
                [&](RandomEngine<T>& random_engine)
                {
                        return cosine_on_hemisphere(random_engine, normal);
                });

        test_distribution_angle<N, T>(
                name, ANGLE_COUNT_PER_BUCKET, normal,
                [&](RandomEngine<T>& random_engine)
                {
                        return cosine_on_hemisphere(random_engine, normal);
                },
                [](T angle)
                {
                        return cosine_on_hemisphere_pdf<N, T>(std::cos(angle));
                });

        test_distribution_surface<N, T>(
                name, SURFACE_COUNT_PER_BUCKET,
                [&](RandomEngine<T>& random_engine)
                {
                        return cosine_on_hemisphere(random_engine, normal);
                },
                [&](const Vector<N, T>& v)
                {
                        return cosine_on_hemisphere_pdf<N, T>(dot(normal, v));
                });

        test_performance<N, T>(
                name, PERFORMANCE_COUNT,
                [&](RandomEngine<T>& random_engine)
                {
                        return cosine_on_hemisphere(random_engine, normal);
                });
}

template <std::size_t N, typename T>
void test_power_cosine_on_hemisphere()
{
        const T power = []()
        {
                RandomEngine<T> random_engine = create_engine<RandomEngine<T>>();
                return std::uniform_real_distribution<T>(1, 100)(random_engine);
        }();

        const std::string name = "Power Cosine, power = " + to_string_fixed(power, 1);

        const Vector<N, T> normal = []()
        {
                RandomEngine<T> random_engine = create_engine<RandomEngine<T>>();
                return uniform_on_sphere<N, T>(random_engine).normalized();
        }();

        test_unit<N, T>(
                name, UNIT_COUNT,
                [&](RandomEngine<T>& random_engine)
                {
                        return power_cosine_on_hemisphere(random_engine, normal, power);
                });

        test_distribution_angle<N, T>(
                name, ANGLE_COUNT_PER_BUCKET, normal,
                [&](RandomEngine<T>& random_engine)
                {
                        return power_cosine_on_hemisphere(random_engine, normal, power);
                },
                [&](T angle)
                {
                        return power_cosine_on_hemisphere_pdf<N, T>(std::cos(angle), power);
                });

        test_distribution_surface<N, T>(
                name, SURFACE_COUNT_PER_BUCKET,
                [&](RandomEngine<T>& random_engine)
                {
                        return power_cosine_on_hemisphere(random_engine, normal, power);
                },
                [&](const Vector<N, T>& v)
                {
                        return power_cosine_on_hemisphere_pdf<N, T>(dot(normal, v), power);
                });

        test_performance<N, T>(
                name, PERFORMANCE_COUNT,
                [&](RandomEngine<T>& random_engine)
                {
                        return power_cosine_on_hemisphere(random_engine, normal, power);
                });
}

template <std::size_t N, typename T>
void test_ggx()
{
        const T alpha = []()
        {
                RandomEngine<T> random_engine = create_engine<RandomEngine<T>>();
                return std::uniform_real_distribution<T>(0.1, 1)(random_engine);
        }();

        const std::string name = "GGX, alpha = " + to_string_fixed(alpha, 2);

        const Vector<N, T> normal = []()
        {
                RandomEngine<T> random_engine = create_engine<RandomEngine<T>>();
                return uniform_on_sphere<N, T>(random_engine).normalized();
        }();

        test_unit<N, T>(
                name, UNIT_COUNT,
                [&](RandomEngine<T>& random_engine)
                {
                        Vector<N, T> v = uniform_on_sphere<N, T>(random_engine);
                        if (dot(v, normal) < 0)
                        {
                                v = -v;
                        }
                        return ggx_vn(random_engine, normal, v, alpha);
                });

        test_distribution_angle<N, T>(
                name + ", Normals", ANGLE_COUNT_PER_BUCKET, normal,
                [&](RandomEngine<T>& random_engine)
                {
                        return ggx_vn(random_engine, normal, normal, alpha);
                },
                [&](T angle)
                {
                        return ggx_pdf(std::cos(angle), alpha);
                });

        test_distribution_surface<N, T>(
                name + ", Normals", SURFACE_COUNT_PER_BUCKET,
                [&](RandomEngine<T>& random_engine)
                {
                        return ggx_vn(random_engine, normal, normal, alpha);
                },
                [&](const Vector<N, T>& v)
                {
                        return ggx_pdf(dot(normal, v), alpha);
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

        test_distribution_surface<N, T>(
                name + ", Visible Normals", SURFACE_COUNT_PER_BUCKET,
                [&](RandomEngine<T>& random_engine)
                {
                        return ggx_vn(random_engine, normal, v, alpha);
                },
                [&](const Vector<N, T>& h)
                {
                        const T n_h = dot(normal, h);
                        const T h_v = dot(h, v);
                        return ggx_vn_pdf(n_v, n_h, h_v, alpha);
                });

        test_distribution_surface<N, T>(
                name + ", Visible Normals, Reflected", SURFACE_COUNT_PER_BUCKET,
                [&](RandomEngine<T>& random_engine)
                {
                        const Vector<N, T> h = ggx_vn(random_engine, normal, v, alpha);
                        return numerical::reflect_vn(v, h);
                },
                [&](const Vector<N, T>& l)
                {
                        const Vector<N, T> h = (l + v).normalized();
                        const T n_h = dot(normal, h);
                        const T h_v = dot(h, v);
                        return ggx_vn_reflected_pdf(n_v, n_h, h_v, alpha);
                });

        test_performance<N, T>(
                name, PERFORMANCE_COUNT,
                [&](RandomEngine<T>& random_engine)
                {
                        return ggx_vn(random_engine, normal, v, alpha);
                });
}

template <std::size_t N, typename T>
void test_distribution()
{
        test_uniform_on_sphere<N, T>();
        LOG("");

        test_cosine_on_hemisphere<N, T>();
        LOG("");

        test_power_cosine_on_hemisphere<N, T>();
        LOG("");

        if constexpr (N == 3)
        {
                test_ggx<N, T>();
                LOG("");
        }
}

template <std::size_t N>
void test_distribution()
{
        test_distribution<N, float>();
        test_distribution<N, double>();
}

void test_distribution_3()
{
        test_distribution<3>();
}
void test_distribution_4()
{
        test_distribution<4>();
}
void test_distribution_5()
{
        test_distribution<5>();
}

TEST_LARGE("Sample Distribution 3-Space", test_distribution_3)
TEST_LARGE("Sample Distribution 4-Space", test_distribution_4)
TEST_LARGE("Sample Distribution 5-Space", test_distribution_5)
}
}
