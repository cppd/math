/*
Copyright (C) 2017-2026 Topological Manifold

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
#include <src/com/interpolation_smooth.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/random/pcg.h>
#include <src/com/type/limit.h>
#include <src/com/type/name.h>
#include <src/test/test.h>

#include <array>
#include <cmath>
#include <random>
#include <string>
#include <vector>

namespace ns
{
namespace
{
template <typename T, Smooth SMOOTH>
struct Check1
{
        static constexpr T MAX = Limits<T>::max() / 2;
        static_assert(interpolation<SMOOTH, T>(1, MAX, T{0}) == 1);
        static_assert(interpolation<SMOOTH, T>(1, MAX, T{1}) == MAX);
        static_assert(interpolation<SMOOTH, T>(MAX, 1, T{0}) == MAX);
        static_assert(interpolation<SMOOTH, T>(MAX, 1, T{1}) == 1);
        static_assert(interpolation<SMOOTH, T>(1, MAX, T{1} / 2) == MAX / 2);
        static_assert(interpolation<SMOOTH, T>(MAX, 1, T{1} / 2) == MAX / 2);
};

template <typename T>
struct Check2 final
        : Check1<T, Smooth::N_0>,
          Check1<T, Smooth::N_1>,
          Check1<T, Smooth::N_2>,
          Check1<T, Smooth::N_3>,
          Check1<T, Smooth::N_4>
{
};

template struct Check2<float>;
template struct Check2<double>;
template struct Check2<long double>;

template <typename T, typename RandomEngine>
std::vector<std::array<T, 3>> make_random_data(const int count, RandomEngine& engine)
{
        std::uniform_real_distribution<T> urd(0, 1);
        std::vector<std::array<T, 3>> res;
        res.reserve(count);
        for (int i = 0; i < count; ++i)
        {
                res.push_back(std::array{urd(engine), urd(engine), urd(engine)});
        }
        return res;
}

template <int ITERATION_COUNT, typename T, Smooth SMOOTH, typename RandomEngine>
void test_performance(RandomEngine& engine)
{
        static constexpr int DATA_COUNT = 1'000'000;

        const std::vector<std::array<T, 3>> data = make_random_data<T>(DATA_COUNT, engine);

        const Clock::time_point start_time = Clock::now();
        for (const auto& v : data)
        {
                for (int i = 0; i < ITERATION_COUNT; ++i)
                {
                        do_not_optimize(interpolation<SMOOTH>(v[0], v[1], v[2]));
                }
        }
        const auto performance = std::llround(DATA_COUNT * (ITERATION_COUNT / duration_from(start_time)));

        LOG(std::string("Smooth Interpolation<") + type_name<T>() + ", " + std::string(smooth_to_string(SMOOTH))
            + ">: " + to_string_digit_groups(performance) + " o/s");
}

template <int ITERATION_COUNT, typename T, typename RandomEngine>
void test_performance(RandomEngine& engine)
{
        test_performance<ITERATION_COUNT, T, Smooth::N_0>(engine);
        test_performance<ITERATION_COUNT, T, Smooth::N_1>(engine);
        test_performance<ITERATION_COUNT, T, Smooth::N_2>(engine);
        test_performance<ITERATION_COUNT, T, Smooth::N_3>(engine);
        test_performance<ITERATION_COUNT, T, Smooth::N_4>(engine);
}

void test()
{
        PCG engine;

        test_performance<1000, float>(engine);
        test_performance<1000, double>(engine);
        test_performance<500, long double>(engine);
}

TEST_PERFORMANCE("Smooth Interpolation", test)
}
}
