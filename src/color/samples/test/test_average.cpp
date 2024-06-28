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

#include <src/color/samples/average.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/random/pcg.h>
#include <src/numerical/integrate.h>
#include <src/test/test.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <ios>
#include <random>
#include <span>
#include <sstream>
#include <type_traits>
#include <vector>

namespace ns::color::samples
{
namespace
{
template <typename T>
void compare(const std::vector<T>& a, const std::vector<T>& b)
{
        if (a.size() != b.size())
        {
                error("Size error " + to_string(a.size()) + ", " + to_string(b.size()));
        }

        for (std::size_t i = 0; i < a.size(); ++i)
        {
                if (a[i] == b[i])
                {
                        continue;
                }

                const T absolute = std::abs(a[i] - b[i]);
                if (!(absolute <= T{0.01}))
                {
                        error(to_string(a[i]) + " and " + to_string(b[i])
                              + " are not equal, absolute error = " + to_string(absolute));
                }

                if (std::abs(a[i]) <= T{0.1} || std::abs(b[i]) <= T{0.1})
                {
                        continue;
                }

                const T relative = absolute / std::max(std::abs(a[i]), std::abs(b[i]));
                if (!(relative <= T{0.01}))
                {
                        error(to_string(a[i]) + " and " + to_string(b[i])
                              + " are not equal, relative error = " + to_string(relative));
                }
        }
}

template <typename Container>
Container::value_type f(const typename Container::value_type wave, const Container& waves, const Container& samples)
{
        using T = Container::value_type;

        if (wave < waves.front() || wave > waves.back())
        {
                return 0;
        }

        const auto iter = std::lower_bound(waves.begin(), waves.end(), wave);
        ASSERT(iter != waves.end());

        const std::size_t index = iter - waves.begin();
        if (waves[index] == wave)
        {
                return samples[index];
        }
        ASSERT(index != 0);

        const T k = (wave - waves[index - 1]) / (waves[index] - waves[index - 1]);
        return std::lerp(samples[index - 1], samples[index], k);
}

template <typename Result, typename Container>
void check(
        const Container& waves,
        const Container& samples,
        const typename Container::value_type from,
        const typename Container::value_type to,
        const unsigned count)
{
        using T = Container::value_type;

        ASSERT(std::is_sorted(waves.begin(), waves.end()));

        const auto function = [&](const T wave)
        {
                return f(wave, waves, samples);
        };

        const std::vector<Result> averages = average<Result>(waves, samples, from, to, count);
        if (!(averages.size() == count))
        {
                error("Result size " + to_string(averages.size()) + " is not equal to " + to_string(count));
        }

        std::vector<Result> test_averages;
        test_averages.reserve(averages.size());

        constexpr int INTEGRATE_COUNT = 10000;

        T a = from;
        for (std::size_t i = 1; i <= averages.size(); ++i)
        {
                const T b = std::lerp(from, to, static_cast<T>(i) / averages.size());
                const T integral = numerical::integrate(function, a, b, INTEGRATE_COUNT);
                const T average = integral / (b - a);
                test_averages.push_back(average);
                a = b;
        }

        compare(averages, test_averages);
}

template <typename ResultType, typename T>
void test_constant()
{
        constexpr std::array<T, 3> WAVES{2, 4, 6};
        constexpr std::array<T, 3> SAMPLES{1, 1, 1};

        compare(average<ResultType>(WAVES, SAMPLES, 0, 10, 1), {0.4});
        check<ResultType>(WAVES, SAMPLES, 0, 10, 1);

        compare(average<ResultType>(WAVES, SAMPLES, 1, 3, 1), {0.5});
        check<ResultType>(WAVES, SAMPLES, 0, 3, 1);

        compare(average<ResultType>(WAVES, SAMPLES, 5, 7, 1), {0.5});
        check<ResultType>(WAVES, SAMPLES, 5, 7, 1);

        compare(average<ResultType>(WAVES, SAMPLES, 3, 5, 1), {1});
        check<ResultType>(WAVES, SAMPLES, 3, 5, 1);

        compare(average<ResultType>(WAVES, SAMPLES, 0, 10, 4), {0.2, 1, 0.4, 0});
        check<ResultType>(WAVES, SAMPLES, 0, 10, 4);

        compare(average<ResultType>(WAVES, SAMPLES, 4, 6, 3), {1, 1, 1});
        check<ResultType>(WAVES, SAMPLES, 4, 6, 3);

        compare(average<ResultType>(WAVES, SAMPLES, 6, 8, 3), {0, 0, 0});
        check<ResultType>(WAVES, SAMPLES, 6, 8, 3);

        compare(average<ResultType>(WAVES, SAMPLES, 0, 2, 3), {0, 0, 0});
        check<ResultType>(WAVES, SAMPLES, 0, 2, 3);

        compare(average<ResultType>(WAVES, SAMPLES, 0, 2.5, 5), {0, 0, 0, 0, 1});
        check<ResultType>(WAVES, SAMPLES, 0, 2.5, 5);

        compare(average<ResultType>(WAVES, SAMPLES, 5.5, 8, 5), {1, 0, 0, 0, 0});
        check<ResultType>(WAVES, SAMPLES, 5.5, 8, 5);
}

template <typename T, typename RandomEngine>
std::array<T, 2> min_max(
        std::type_identity_t<T> from,
        std::type_identity_t<T> to,
        std::type_identity_t<T> min_distance,
        RandomEngine& engine)
{
        ASSERT(from < to);
        ASSERT(min_distance < (to - from));

        T min;
        T max;
        do
        {
                min = std::uniform_real_distribution<T>(from, to)(engine);
                max = std::uniform_real_distribution<T>(from, to)(engine);
        } while (!(std::abs(min - max) > min_distance));

        if (min > max)
        {
                std::swap(min, max);
        }
        ASSERT((max - min) > min_distance && min >= from && max <= to);

        return {min, max};
}

template <typename ResultType, typename T>
void test_random()
{
        PCG engine;

        constexpr unsigned MIN_COUNT = 10;
        constexpr unsigned MAX_COUNT = 100;
        const unsigned wave_count = std::uniform_int_distribution<unsigned>(MIN_COUNT, MAX_COUNT)(engine);
        const unsigned test_count = std::uniform_int_distribution<unsigned>(MIN_COUNT, MAX_COUNT)(engine);

        constexpr T WAVE_MIN = 0;
        constexpr T WAVE_MAX = 1000;
        constexpr T WAVE_DISTANCE = 1;
        const auto [wave_min, wave_max] = min_max<T>(WAVE_MIN, WAVE_MAX, WAVE_DISTANCE, engine);
        const auto [test_min, test_max] = min_max<T>(WAVE_MIN, WAVE_MAX, WAVE_DISTANCE, engine);

        constexpr T SAMPLE_MIN = 0;
        constexpr T SAMPLE_MAX = 10;
        constexpr T SAMPLE_DISTANCE = 1;
        const auto [sample_min, sample_max] = min_max<T>(SAMPLE_MIN, SAMPLE_MAX, SAMPLE_DISTANCE, engine);

        std::vector<T> waves;
        waves.reserve(wave_count);
        std::vector<T> samples;
        samples.reserve(wave_count);

        for (unsigned i = 0; i < wave_count; ++i)
        {
                waves.push_back(std::uniform_real_distribution<T>(wave_min, wave_max)(engine));
                samples.push_back(std::uniform_real_distribution<T>(sample_min, sample_max)(engine));
        }

        std::sort(waves.begin(), waves.end());

        std::ostringstream oss;
        oss << std::fixed;
        oss << "samples " << waves.size() << " [" << waves.front() << ", " << waves.back() << "]; ";
        oss << "test " << test_count << " [" << test_min << ", " << test_max << "]";
        LOG(oss.str());

        check<ResultType>(waves, samples, wave_min, wave_max, wave_count);
        check<ResultType>(waves, samples, test_min, test_max, test_count);
        check<ResultType>(std::span<const T>(waves), std::span<const T>(samples), test_min, test_max, test_count);
}

void test()
{
        LOG("Test average samples");

        test_constant<float, float>();
        test_constant<float, double>();
        test_constant<double, float>();
        test_constant<double, double>();

        for (int i = 0; i < 2; ++i)
        {
                test_random<float, float>();
                test_random<float, double>();
                test_random<double, float>();
                test_random<double, double>();
        }

        LOG("Test average samples passed");
}

TEST_SMALL("Average Samples", test)
}
}
