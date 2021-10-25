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

#include "../fresnel.h"

#include <src/com/benchmark.h>
#include <src/com/chrono.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/random/engine.h>
#include <src/com/type/name.h>
#include <src/sampling/sphere_uniform.h>
#include <src/test/test.h>

#include <cstddef>
#include <random>
#include <span>
#include <vector>

namespace ns::painter
{
namespace
{
template <std::size_t N, typename T>
std::vector<Vector<N, T>> random_data(const int count, std::mt19937_64& engine)
{
        std::vector<Vector<N, T>> data;
        data.reserve(count);
        for (int i = 0; i < count; ++i)
        {
                data.push_back(sampling::uniform_on_sphere<N, T>(engine));
        }
        return data;
}

template <int COUNT, std::size_t N, typename T, typename F>
double test(const std::vector<Vector<N, T>> data, const F& f)
{
        Clock::time_point start_time = Clock::now();
        for (int i = 0; i < COUNT; ++i)
        {
                for (const Vector<N, T>& v : data)
                {
                        do_not_optimize(f(v));
                }
        }
        return duration_from(start_time);
}

template <std::size_t N, typename T>
void test_fresnel_performance()
{
        constexpr int DATA_SIZE = 10'000;
        constexpr int COUNT = 10'000;

        constexpr T N_1 = 1;
        constexpr T N_2 = 1.5;
        constexpr T ETA = N_1 / N_2;
        constexpr T K = T(0.5);

        std::mt19937_64 engine = create_engine<std::mt19937_64>();

        const Vector<N, T> normal = sampling::uniform_on_sphere<N, T>(engine);
        const std::vector<Vector<N, T>> data = random_data<N, T>(DATA_SIZE, engine);

        const double dielectric = test<COUNT>(
                data,
                [&](const Vector<N, T>& v)
                {
                        return fresnel_dielectric(v, normal, N_1, N_2);
                });

        const double conductor = test<COUNT>(
                data,
                [&](const Vector<N, T>& v)
                {
                        return fresnel_conductor(v, normal, ETA, K);
                });

        LOG("Fresnel <" + to_string(N) + ", " + type_name<T>() + ">: dielectric " + to_string_fixed(dielectric, 5)
            + " s, conductor " + to_string_fixed(conductor, 5) + " s");
}

template <typename T>
void test_fresnel_performance()
{
        test_fresnel_performance<2, T>();
        test_fresnel_performance<3, T>();
        test_fresnel_performance<4, T>();
        test_fresnel_performance<5, T>();
}

void test_fresnel()
{
        test_fresnel_performance<float>();
        test_fresnel_performance<double>();
}

TEST_PERFORMANCE("Fresnel", test_fresnel)
}
}
