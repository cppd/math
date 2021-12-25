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

#include <src/com/log.h>
#include <src/com/names.h>
#include <src/com/print.h>
#include <src/com/random/pcg.h>
#include <src/com/type/name.h>
#include <src/sampling/testing/test.h>
#include <src/test/test.h>

#include <cmath>
#include <random>
#include <sstream>

namespace ns::shading::test
{
namespace
{
constexpr long long UNIT_COUNT = 10'000'000;
constexpr long long ANGLE_COUNT_PER_BUCKET = 1'000;
constexpr long long SURFACE_COUNT_PER_BUCKET = 10'000;
constexpr long long PERFORMANCE_COUNT = 10'000'000;

namespace st = sampling::testing;

template <typename T>
T random_alpha()
{
        PCG engine;
        return std::uniform_real_distribution<T>(0.1, 1)(engine);
}

template <std::size_t N, typename T>
Vector<N, T> random_normal()
{
        PCG engine;
        return sampling::uniform_on_sphere<N, T>(engine).normalized();
}

template <std::size_t N, typename T>
Vector<N, T> random_v(const Vector<N, T>& normal)
{
        PCG engine;
        Vector<N, T> r = sampling::uniform_on_sphere<N, T>(engine).normalized();
        if (dot(r, normal) < 0)
        {
                return -r;
        }
        return r;
}

//

template <std::size_t N, typename T>
void test_ggx(ProgressRatio* const progress)
{
        const T alpha = random_alpha<T>();

        LOG("GGX, " + space_name(N) + ", " + type_name<T>() + ", alpha " + to_string_fixed(alpha, 2));

        const Vector<N, T> normal = random_normal<N, T>();

        st::test_unit<N, T>(
                "Visible Normals", UNIT_COUNT,
                [&](auto& engine)
                {
                        Vector<N, T> v = sampling::uniform_on_sphere<N, T>(engine);
                        if (dot(v, normal) < 0)
                        {
                                v = -v;
                        }
                        return ggx_visible_normals_h(engine, normal, v, alpha);
                },
                progress);

        st::test_unit<N, T>(
                "Visible Normals, Reflected", UNIT_COUNT,
                [&](auto& engine)
                {
                        Vector<N, T> v = sampling::uniform_on_sphere<N, T>(engine);
                        if (dot(v, normal) < 0)
                        {
                                v = -v;
                        }
                        const auto [h, l] = ggx_visible_normals_h_l(engine, normal, v, alpha);
                        return l;
                },
                progress);

        st::test_distribution_angle<N, T>(
                "Normals", ANGLE_COUNT_PER_BUCKET, normal,
                [&](auto& engine)
                {
                        return ggx_visible_normals_h(engine, normal, normal, alpha);
                },
                [&](const T angle)
                {
                        const T n_h = std::cos(angle);
                        return n_h * ggx_pdf<N>(n_h, alpha);
                },
                progress);

        st::test_distribution_surface<N, T>(
                "Normals", SURFACE_COUNT_PER_BUCKET,
                [&](auto& engine)
                {
                        return ggx_visible_normals_h(engine, normal, normal, alpha);
                },
                [&](const Vector<N, T>& v)
                {
                        const T n_h = dot(normal, v);
                        return n_h * ggx_pdf<N>(n_h, alpha);
                },
                progress);

        const Vector<N, T> v = random_v<N, T>(normal);

        const T n_v = dot(normal, v);

        st::test_distribution_surface<N, T>(
                "Visible Normals", SURFACE_COUNT_PER_BUCKET,
                [&](auto& engine)
                {
                        return ggx_visible_normals_h(engine, normal, v, alpha);
                },
                [&](const Vector<N, T>& h)
                {
                        const T n_h = dot(normal, h);
                        const T h_v = dot(h, v);
                        return ggx_visible_normals_h_pdf<N>(n_v, n_h, h_v, alpha);
                },
                progress);

        st::test_distribution_surface<N, T>(
                "Visible Normals, Reflected", SURFACE_COUNT_PER_BUCKET,
                [&](auto& engine)
                {
                        const auto [h, l] = ggx_visible_normals_h_l(engine, normal, v, alpha);
                        return l;
                },
                [&](const Vector<N, T>& l)
                {
                        const Vector<N, T> h = (l + v).normalized();
                        const T n_h = dot(normal, h);
                        const T h_v = dot(h, v);
                        return ggx_visible_normals_l_pdf<N>(n_v, n_h, h_v, alpha);
                },
                progress);

        st::test_performance<PERFORMANCE_COUNT>(
                "Visible Normals",
                [&](auto& engine)
                {
                        return ggx_visible_normals_h(engine, normal, v, alpha);
                },
                progress);

        st::test_performance<PERFORMANCE_COUNT>(
                "Visible Normals, Reflected",
                [&](auto& engine)
                {
                        const auto [h, l] = ggx_visible_normals_h_l(engine, normal, v, alpha);
                        return l;
                },
                progress);
}

template <std::size_t N>
void test_ggx(ProgressRatio* const progress)
{
        test_ggx<N, float>(progress);
        test_ggx<N, double>(progress);
}

//

template <std::size_t N, typename T>
void test_performance()
{
        const T alpha = random_alpha<T>();
        const Vector<N, T> normal = random_normal<N, T>();
        const Vector<N, T> v = random_v<N, T>(normal);

        const long long p_visible_normals = st::test_performance<PERFORMANCE_COUNT>(
                [&](auto& engine)
                {
                        return ggx_visible_normals_h(engine, normal, v, alpha);
                });

        const long long p_visible_normals_reflected = st::test_performance<PERFORMANCE_COUNT>(
                [&](auto& engine)
                {
                        const auto [h, l] = ggx_visible_normals_h_l(engine, normal, v, alpha);
                        return l;
                });

        std::ostringstream oss;
        oss << "GGX visible normals <" << N << ", " << type_name<T>() << ">: ";
        oss << to_string_digit_groups(p_visible_normals) << " o/s, reflected ";
        oss << to_string_digit_groups(p_visible_normals_reflected) << " o/s";
        LOG(oss.str());
}

template <typename T, typename Counter>
void test_performance(const Counter& counter)
{
        counter();
        test_performance<3, T>();
        counter();
        test_performance<4, T>();
        counter();
        test_performance<5, T>();
}

void test_ggx_performance(ProgressRatio* const progress)
{
        constexpr int COUNT = 3 * 2;
        int i = -1;
        const auto counter = [&]
        {
                progress->set(++i, COUNT);
        };
        test_performance<float>(counter);
        test_performance<double>(counter);
}

//

TEST_LARGE("Sample Distribution, GGX, 3-Space", test_ggx<3>)
TEST_LARGE("Sample Distribution, GGX, 4-Space", test_ggx<4>)
TEST_LARGE("Sample Distribution, GGX, 5-Space", test_ggx<5>)

TEST_PERFORMANCE("Sampling, GGX", test_ggx_performance)
}
}
