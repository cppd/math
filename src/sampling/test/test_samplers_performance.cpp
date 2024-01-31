/*
Copyright (C) 2017-2024 Topological Manifold

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
#include <src/com/exponent.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/random/create.h>
#include <src/com/random/name.h>
#include <src/com/random/pcg.h>
#include <src/com/type/name.h>
#include <src/numerical/vector.h>
#include <src/progress/progress.h>
#include <src/sampling/halton_sampler.h>
#include <src/sampling/lh_sampler.h>
#include <src/sampling/sj_sampler.h>
#include <src/test/test.h>

#include <cmath>
#include <cstddef>
#include <random>
#include <sstream>
#include <vector>

namespace ns::sampling::test
{
namespace
{
template <std::size_t N>
constexpr int one_dimension_sample_count()
{
        static_assert(N >= 2);
        switch (N)
        {
        case 2:
        case 3:
                return 5;
        case 4:
                return 4;
        case 5:
        case 6:
                return 3;
        default:
                return 2;
        }
}

template <std::size_t N, typename T, typename RandomEngine>
void test_performance(const bool shuffle)
{
        auto engine = create_engine<RandomEngine>();

        constexpr int ITER_COUNT = 100'000;
        constexpr int SAMPLE_COUNT = power<N>(one_dimension_sample_count<N>());
        constexpr long long COUNT = static_cast<long long>(ITER_COUNT) * SAMPLE_COUNT;

        const long long sjs = [&]
        {
                std::vector<numerical::Vector<N, T>> data;
                const StratifiedJitteredSampler<N, T> sampler(0, 1, SAMPLE_COUNT, shuffle);
                const Clock::time_point start_time = Clock::now();
                for (int i = 0; i < ITER_COUNT; ++i)
                {
                        sampler.generate(engine, &data);
                }
                return std::llround(COUNT / duration_from(start_time));
        }();

        const long long lhs = [&]
        {
                std::vector<numerical::Vector<N, T>> data;
                const LatinHypercubeSampler<N, T> sampler(0, 1, SAMPLE_COUNT, shuffle);
                const Clock::time_point start_time = Clock::now();
                for (int i = 0; i < ITER_COUNT; ++i)
                {
                        sampler.generate(engine, &data);
                }
                return std::llround(COUNT / duration_from(start_time));
        }();

        const long long hs = [&]
        {
                HaltonSampler<N, T> sampler;
                const Clock::time_point start_time = Clock::now();
                for (int i = 0; i < ITER_COUNT; ++i)
                {
                        for (int j = 0; j < SAMPLE_COUNT; ++j)
                        {
                                do_not_optimize(sampler.generate());
                        }
                }
                return std::llround(COUNT / duration_from(start_time));
        }();

        std::ostringstream oss;
        oss << "Samplers <" << N << ", " << type_name<T>() << ", " << random_engine_name<RandomEngine>() << ">";
        if (shuffle)
        {
                oss << ", shuffle";
        }
        else
        {
                oss << "         ";
        }
        oss << ":";

        oss << " SJS = " << to_string_digit_groups(sjs) << " o/s";
        oss << ", LHS = " << to_string_digit_groups(lhs) << " o/s";
        oss << ", HS = " << to_string_digit_groups(hs) << " o/s";
        LOG(oss.str());
}

template <std::size_t N, typename T, typename RandomEngine, typename Counter>
void test_performance(const Counter& counter)
{
        counter();
        test_performance<N, T, RandomEngine>(false);
        counter();
        test_performance<N, T, RandomEngine>(true);
}

template <typename T, typename RandomEngine, typename Counter>
void test_performance(const Counter& counter)
{
        static_assert(std::is_floating_point_v<T>);

        test_performance<2, T, RandomEngine>(counter);
        test_performance<3, T, RandomEngine>(counter);
        test_performance<4, T, RandomEngine>(counter);
        test_performance<5, T, RandomEngine>(counter);
        test_performance<6, T, RandomEngine>(counter);
}

template <typename T, typename Counter>
void test_performance(const Counter& counter)
{
        test_performance<T, std::mt19937>(counter);
        test_performance<T, std::mt19937_64>(counter);
        test_performance<T, PCG>(counter);
}

void test_sampler_performance(progress::Ratio* const progress)
{
        constexpr int COUNT = 2 * 5 * 3 * 3;
        int i = -1;
        const auto counter = [&]
        {
                progress->set(++i, COUNT);
        };
        test_performance<float>(counter);
        test_performance<double>(counter);
        test_performance<long double>(counter);
}

TEST_PERFORMANCE("Samplers", test_sampler_performance)
}
}
