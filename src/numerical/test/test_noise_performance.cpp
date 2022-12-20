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

#include "../noise.h"

#include <src/com/benchmark.h>
#include <src/com/chrono.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/random/pcg.h>
#include <src/com/type/name.h>
#include <src/settings/dimensions.h>
#include <src/test/test.h>

#include <random>
#include <vector>

namespace ns::numerical
{
namespace
{
template <std::size_t N, typename T>
std::vector<Vector<N, T>> random_data(const int count)
{
        PCG engine;
        std::uniform_real_distribution<T> urd(-10, 10);

        std::vector<Vector<N, T>> res;
        res.reserve(count);
        for (int i = 0; i < count; ++i)
        {
                Vector<N, T>& v = res.emplace_back();
                for (std::size_t n = 0; n < N; ++n)
                {
                        v[n] = urd(engine);
                }
        }
        return res;
}

template <std::size_t N, typename T>
void test_performance()
{
        constexpr int DATA_COUNT = 1'000'000;
        constexpr int COUNT = 32;

        const std::vector<Vector<N, T>> data = random_data<N, T>(DATA_COUNT);

        const Clock::time_point start_time = Clock::now();
        for (const Vector<N, T>& v : data)
        {
                for (int i = 0; i < COUNT; ++i)
                {
                        do_not_optimize(noise(v));
                }
        }
        const long long performance = std::llround(COUNT * (data.size() / duration_from(start_time)));

        std::ostringstream oss;
        oss << "Noise <" << N << ", " << type_name<T>() << ">: " << to_string_digit_groups(performance) << " o/s";
        LOG(oss.str());
}

template <std::size_t... I>
void test_performance(std::index_sequence<I...>&&)
{
        (test_performance<I, float>(), ...);
        (test_performance<I, double>(), ...);
}

void test_noise_performance()
{
        test_performance(settings::Dimensions2());
}

TEST_PERFORMANCE("Noise", test_noise_performance)
}
}
