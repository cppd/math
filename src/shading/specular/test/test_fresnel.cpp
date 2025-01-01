/*
Copyright (C) 2017-2025 Topological Manifold

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

#include <src/com/benchmark.h>
#include <src/com/chrono.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/random/pcg.h>
#include <src/com/type/name.h>
#include <src/numerical/vector.h>
#include <src/sampling/sphere_uniform.h>
#include <src/shading/specular/fresnel.h>
#include <src/test/test.h>

#include <cmath>
#include <cstddef>
#include <sstream>
#include <vector>

namespace ns::shading::specular
{
namespace
{
template <std::size_t N, typename T, typename RandomEngine>
std::vector<numerical::Vector<N, T>> random_data(const int count, RandomEngine& engine)
{
        std::vector<numerical::Vector<N, T>> res;
        res.reserve(count);
        for (int i = 0; i < count; ++i)
        {
                res.push_back(sampling::uniform_on_sphere<N, T>(engine));
        }
        return res;
}

template <int COUNT, std::size_t N, typename T, typename F>
long long test(const std::vector<numerical::Vector<N, T>>& data, const F& f)
{
        const Clock::time_point start_time = Clock::now();
        for (int i = 0; i < COUNT; ++i)
        {
                for (const numerical::Vector<N, T>& v : data)
                {
                        do_not_optimize(f(v));
                }
        }
        return std::llround(COUNT * (data.size() / duration_from(start_time)));
}

template <std::size_t N, typename T>
void test_fresnel_performance()
{
        constexpr int DATA_SIZE = 10'000;
        constexpr int COUNT = 10'000;

        constexpr T N_1 = 1;
        constexpr T N_2 = 1.5;
        constexpr T ETA = N_1 / N_2;
        constexpr T K = 0.5;

        PCG engine;

        const numerical::Vector<N, T> normal = sampling::uniform_on_sphere<N, T>(engine);
        const std::vector<numerical::Vector<N, T>> data = random_data<N, T>(DATA_SIZE, engine);

        std::ostringstream oss;
        oss << "Fresnel <" << N << ", " << type_name<T>() << ">:";

        {
                const auto p = test<COUNT>(
                        data,
                        [&](const numerical::Vector<N, T>& v)
                        {
                                return fresnel_dielectric(v, normal, N_1, N_2);
                        });
                oss << " dielectric = " << to_string_digit_groups(p) + " o/s";
        }
        {
                const auto p = test<COUNT>(
                        data,
                        [&](const numerical::Vector<N, T>& v)
                        {
                                return fresnel_conductor(v, normal, ETA, K);
                        });
                oss << ", conductor = " << to_string_digit_groups(p) << " o/s";
        }

        LOG(oss.str());
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
