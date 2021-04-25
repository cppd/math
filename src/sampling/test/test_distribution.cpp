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

#include <src/com/constant.h>
#include <src/com/log.h>
#include <src/com/random/engine.h>
#include <src/geometry/shapes/sphere_area.h>
#include <src/numerical/optics.h>
#include <src/test/test.h>

#include <cmath>
#include <random>
#include <string>

namespace ns::sampling::test
{
namespace
{
template <std::size_t N, typename T>
void test_uniform_on_sphere(
        long long unit_count,
        long long angle_distribution_count,
        long long surface_distribution_count,
        long long performance_count)
{
        const std::string NAME = "Uniform";

        const Vector<N, T> NORMAL = []()
        {
                RandomEngine<T> random_engine = create_engine<RandomEngine<T>>();
                return uniform_on_sphere<N, T>(random_engine).normalized();
        }();

        test_unit<N, T>(
                NAME, unit_count,
                [&](RandomEngine<T>& random_engine)
                {
                        return uniform_on_sphere<N, T>(random_engine);
                });

        test_distribution_angle<N, T>(
                NAME, angle_distribution_count, NORMAL,
                [&](RandomEngine<T>& random_engine)
                {
                        return uniform_on_sphere<N, T>(random_engine);
                },
                [](T /*angle*/)
                {
                        return uniform_on_sphere_pdf<N, T>();
                });

        test_distribution_surface<N, T>(
                NAME, surface_distribution_count,
                [&](RandomEngine<T>& random_engine)
                {
                        return uniform_on_sphere<N, T>(random_engine);
                },
                [&](const Vector<N, T>& /*v*/)
                {
                        return uniform_on_sphere_pdf<N, T>();
                });

        test_performance<N, T>(
                NAME, performance_count,
                [&](RandomEngine<T>& random_engine)
                {
                        return uniform_on_sphere<N, T>(random_engine);
                });
}

template <std::size_t N, typename T>
void test_cosine_on_hemisphere(
        long long unit_count,
        long long angle_distribution_count,
        long long surface_distribution_count,
        long long performance_count)
{
        const std::string NAME = "Cosine";

        const Vector<N, T> NORMAL = []()
        {
                RandomEngine<T> random_engine = create_engine<RandomEngine<T>>();
                return uniform_on_sphere<N, T>(random_engine).normalized();
        }();

        test_unit<N, T>(
                NAME, unit_count,
                [&](RandomEngine<T>& random_engine)
                {
                        return cosine_on_hemisphere(random_engine, NORMAL);
                });

        test_distribution_angle<N, T>(
                NAME, angle_distribution_count, NORMAL,
                [&](RandomEngine<T>& random_engine)
                {
                        return cosine_on_hemisphere(random_engine, NORMAL);
                },
                [](T angle)
                {
                        return cosine_on_hemisphere_pdf<N, T>(std::cos(angle));
                });

        test_distribution_surface<N, T>(
                NAME, surface_distribution_count,
                [&](RandomEngine<T>& random_engine)
                {
                        return cosine_on_hemisphere(random_engine, NORMAL);
                },
                [&](const Vector<N, T>& v)
                {
                        return cosine_on_hemisphere_pdf<N, T>(dot(NORMAL, v));
                });

        test_performance<N, T>(
                NAME, performance_count,
                [&](RandomEngine<T>& random_engine)
                {
                        return cosine_on_hemisphere(random_engine, NORMAL);
                });
}

template <std::size_t N, typename T>
void test_power_cosine_on_hemisphere(
        long long unit_count,
        long long angle_distribution_count,
        long long surface_distribution_count,
        long long performance_count)
{
        const T POWER = []()
        {
                RandomEngine<T> random_engine = create_engine<RandomEngine<T>>();
                return std::uniform_real_distribution<T>(1, 100)(random_engine);
        }();

        const std::string NAME = "Power Cosine, power = " + to_string_fixed(POWER, 1);

        const Vector<N, T> NORMAL = []()
        {
                RandomEngine<T> random_engine = create_engine<RandomEngine<T>>();
                return uniform_on_sphere<N, T>(random_engine).normalized();
        }();

        test_unit<N, T>(
                NAME, unit_count,
                [&](RandomEngine<T>& random_engine)
                {
                        return power_cosine_on_hemisphere(random_engine, NORMAL, POWER);
                });

        test_distribution_angle<N, T>(
                NAME, angle_distribution_count, NORMAL,
                [&](RandomEngine<T>& random_engine)
                {
                        return power_cosine_on_hemisphere(random_engine, NORMAL, POWER);
                },
                [&](T angle)
                {
                        return power_cosine_on_hemisphere_pdf<N, T>(std::cos(angle), POWER);
                });

        test_distribution_surface<N, T>(
                NAME, surface_distribution_count,
                [&](RandomEngine<T>& random_engine)
                {
                        return power_cosine_on_hemisphere(random_engine, NORMAL, POWER);
                },
                [&](const Vector<N, T>& v)
                {
                        return power_cosine_on_hemisphere_pdf<N, T>(dot(NORMAL, v), POWER);
                });

        test_performance<N, T>(
                NAME, performance_count,
                [&](RandomEngine<T>& random_engine)
                {
                        return power_cosine_on_hemisphere(random_engine, NORMAL, POWER);
                });
}

template <std::size_t N, typename T>
void test_ggx(
        long long unit_count,
        long long angle_distribution_count,
        long long surface_distribution_count,
        long long performance_count)
{
        const T ALPHA = []()
        {
                RandomEngine<T> random_engine = create_engine<RandomEngine<T>>();
                return std::uniform_real_distribution<T>(0.1, 1)(random_engine);
        }();

        const std::string NAME = "GGX, alpha = " + to_string_fixed(ALPHA, 2);

        const Vector<N, T> NORMAL = []()
        {
                RandomEngine<T> random_engine = create_engine<RandomEngine<T>>();
                return uniform_on_sphere<N, T>(random_engine).normalized();
        }();

        test_unit<N, T>(
                NAME, unit_count,
                [&](RandomEngine<T>& random_engine)
                {
                        Vector<N, T> v = uniform_on_sphere<N, T>(random_engine);
                        if (dot(v, NORMAL) < 0)
                        {
                                v = -v;
                        }
                        return ggx_vn(random_engine, NORMAL, v, ALPHA);
                });

        test_distribution_angle<N, T>(
                NAME + ", Normals", angle_distribution_count, NORMAL,
                [&](RandomEngine<T>& random_engine)
                {
                        return ggx_vn(random_engine, NORMAL, NORMAL, ALPHA);
                },
                [&](T angle)
                {
                        return ggx_pdf(std::cos(angle), ALPHA);
                });

        test_distribution_surface<N, T>(
                NAME + ", Normals", surface_distribution_count,
                [&](RandomEngine<T>& random_engine)
                {
                        return ggx_vn(random_engine, NORMAL, NORMAL, ALPHA);
                },
                [&](const Vector<N, T>& v)
                {
                        return ggx_pdf(dot(NORMAL, v), ALPHA);
                });

        const Vector<N, T> V = [&]()
        {
                RandomEngine<T> random_engine = create_engine<RandomEngine<T>>();
                Vector<N, T> v = uniform_on_sphere<N, T>(random_engine).normalized();
                if (dot(v, NORMAL) < 0)
                {
                        return -v;
                }
                return v;
        }();

        const T N_V = dot(NORMAL, V);

        test_distribution_surface<N, T>(
                NAME + ", Visible Normals", surface_distribution_count,
                [&](RandomEngine<T>& random_engine)
                {
                        return ggx_vn(random_engine, NORMAL, V, ALPHA);
                },
                [&](const Vector<N, T>& h)
                {
                        const T n_h = dot(NORMAL, h);
                        const T h_v = dot(h, V);
                        return ggx_vn_pdf(N_V, n_h, h_v, ALPHA);
                });

        test_distribution_surface<N, T>(
                NAME + ", Visible Normals, Reflected", surface_distribution_count,
                [&](RandomEngine<T>& random_engine)
                {
                        const Vector<N, T> h = ggx_vn(random_engine, NORMAL, V, ALPHA);
                        return numerical::reflect_vn(V, h);
                },
                [&](const Vector<N, T>& l)
                {
                        const Vector<N, T> h = (l + V).normalized();
                        const T n_h = dot(NORMAL, h);
                        const T h_v = dot(h, V);
                        return ggx_vn_reflected_pdf(N_V, n_h, h_v, ALPHA);
                });

        test_performance<N, T>(
                NAME, performance_count,
                [&](RandomEngine<T>& random_engine)
                {
                        return ggx_vn(random_engine, NORMAL, V, ALPHA);
                });
}

template <std::size_t N, typename T>
long long compute_angle_distribution_count()
{
        const double bucket_size = AngleBuckets<N, T>::bucket_size();
        const double s_all = geometry::sphere_relative_area<N, long double>(0, PI<long double>);
        const double s_bucket = geometry::sphere_relative_area<N, long double>(0, bucket_size);
        const double uniform_min_count = 1000;
        const double count = s_all / s_bucket * uniform_min_count;
        const double round_to = std::pow(10, std::round(std::log10(count)) - 2);
        const double rounded_count = std::ceil(count / round_to) * round_to;
        return (rounded_count <= 1e9 ? rounded_count : 0);
}

template <std::size_t N, typename T>
long long compute_surface_distribution_count()
{
        const double count = 10'000 * SurfaceBuckets<N, T>().bucket_count();
        const double round_to = std::pow(10, std::round(std::log10(count)) - 2);
        return std::ceil(count / round_to) * round_to;
}

template <std::size_t N, typename T>
void test_distribution()
{
        const long long unit_count = 10'000'000;
        const long long angle_distribution_count = compute_angle_distribution_count<N, T>();
        const long long surface_distribution_count = compute_surface_distribution_count<N, T>();
        const long long performance_count = 10'000'000;

        test_uniform_on_sphere<N, T>(
                unit_count, angle_distribution_count, surface_distribution_count, performance_count);
        LOG("");

        test_cosine_on_hemisphere<N, T>(
                unit_count, angle_distribution_count, surface_distribution_count, performance_count);
        LOG("");

        test_power_cosine_on_hemisphere<N, T>(
                unit_count, angle_distribution_count, surface_distribution_count, performance_count);
        LOG("");

        if constexpr (N == 3)
        {
                test_ggx<N, T>(unit_count, angle_distribution_count, surface_distribution_count, performance_count);
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
