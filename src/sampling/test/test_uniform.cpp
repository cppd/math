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
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/random/create.h>
#include <src/com/random/name.h>
#include <src/com/random/pcg.h>
#include <src/com/type/name.h>
#include <src/numerical/vector.h>
#include <src/progress/progress.h>
#include <src/sampling/sphere_uniform.h>
#include <src/test/test.h>

#include <cmath>
#include <cstddef>
#include <random>
#include <sstream>
#include <string>

namespace ns::sampling::test
{
namespace
{
namespace impl = sphere_implementation;

template <std::size_t N, typename T, typename RandomEngine>
numerical::Vector<N, T> uniform_in_sphere_by_rejection(RandomEngine& engine)
{
        numerical::Vector<N, T> v;
        T v_length_square;
        impl::uniform_in_sphere_by_rejection(engine, v, v_length_square);
        return v;
}

template <std::size_t N, typename T, typename RandomEngine>
numerical::Vector<N, T> uniform_in_sphere_by_normal_distribution(RandomEngine& engine)
{
        numerical::Vector<N, T> v;
        T v_length_square;
        impl::uniform_in_sphere_by_normal_distribution(engine, v, v_length_square);
        return v;
}

template <int COUNT, std::size_t N, typename T, typename RandomEngine>
double test_on_sphere_by_rejection(RandomEngine& engine)
{
        const Clock::time_point start_time = Clock::now();
        for (int i = 0; i < COUNT; ++i)
        {
                do_not_optimize(impl::uniform_on_sphere_by_rejection<N, T>(engine));
        }
        return duration_from(start_time);
}

template <int COUNT, std::size_t N, typename T, typename RandomEngine>
double test_on_sphere_by_normal_distribution(RandomEngine& engine)
{
        const Clock::time_point start_time = Clock::now();
        for (int i = 0; i < COUNT; ++i)
        {
                do_not_optimize(impl::uniform_on_sphere_by_normal_distribution<N, T>(engine));
        }
        return duration_from(start_time);
}

template <int COUNT, std::size_t N, typename T, typename RandomEngine>
double test_in_sphere_by_rejection(RandomEngine& engine)
{
        const Clock::time_point start_time = Clock::now();
        for (int i = 0; i < COUNT; ++i)
        {
                do_not_optimize(uniform_in_sphere_by_rejection<N, T>(engine));
        }
        return duration_from(start_time);
}

template <int COUNT, std::size_t N, typename T, typename RandomEngine>
double test_in_sphere_by_normal_distribution(RandomEngine& engine)
{
        const Clock::time_point start_time = Clock::now();
        for (int i = 0; i < COUNT; ++i)
        {
                do_not_optimize(uniform_in_sphere_by_normal_distribution<N, T>(engine));
        }
        return duration_from(start_time);
}

enum class SampleType
{
        ON_SPHERE,
        IN_SPHERE
};

template <SampleType SAMPLE_TYPE>
std::string type_to_string()
{
        static_assert(SAMPLE_TYPE == SampleType::ON_SPHERE || SAMPLE_TYPE == SampleType::IN_SPHERE);
        switch (SAMPLE_TYPE)
        {
        case SampleType::ON_SPHERE:
        {
                return "on sphere";
        }
        case SampleType::IN_SPHERE:
        {
                return "in sphere";
        }
        }
}

template <SampleType SAMPLE_TYPE, std::size_t N, typename T, typename RandomEngine>
void test_performance()
{
        constexpr int COUNT = 3'000'000;

        std::ostringstream oss;

        oss << "Sample " << type_to_string<SAMPLE_TYPE>();
        oss << " <" << N << ", " << type_name<T>() << ", " << random_engine_name<RandomEngine>() << ">:";

        auto engine = create_engine<RandomEngine>();

        static_assert(SAMPLE_TYPE == SampleType::ON_SPHERE || SAMPLE_TYPE == SampleType::IN_SPHERE);
        switch (SAMPLE_TYPE)
        {
        case SampleType::ON_SPHERE:
        {
                const double r = test_on_sphere_by_rejection<COUNT, N, T>(engine);
                oss << " rejection " << to_string_digit_groups(std::llround(COUNT / r)) << " o/s";
                const double n = test_on_sphere_by_normal_distribution<COUNT, N, T>(engine);
                oss << ", normal " << to_string_digit_groups(std::llround(COUNT / n)) << " o/s";
                break;
        }
        case SampleType::IN_SPHERE:
        {
                const double r = test_in_sphere_by_rejection<COUNT, N, T>(engine);
                oss << " rejection " << to_string_digit_groups(std::llround(COUNT / r)) << " o/s";
                const double n = test_in_sphere_by_normal_distribution<COUNT, N, T>(engine);
                oss << ", normal " << to_string_digit_groups(std::llround(COUNT / n)) << " o/s";
                break;
        }
        }
        LOG(oss.str());
}

template <SampleType SAMPLE_TYPE, typename T, typename RandomEngine, typename Counter>
void test_performance(const Counter& counter)
{
        static_assert(std::is_floating_point_v<T>);

        counter();
        test_performance<SAMPLE_TYPE, 2, T, RandomEngine>();
        counter();
        test_performance<SAMPLE_TYPE, 3, T, RandomEngine>();
        counter();
        test_performance<SAMPLE_TYPE, 4, T, RandomEngine>();
        counter();
        test_performance<SAMPLE_TYPE, 5, T, RandomEngine>();
        counter();
        test_performance<SAMPLE_TYPE, 6, T, RandomEngine>();
        counter();
        test_performance<SAMPLE_TYPE, 7, T, RandomEngine>();
}

template <SampleType SAMPLE_TYPE, typename T, typename Counter>
void test_performance(const Counter& counter)
{
        test_performance<SAMPLE_TYPE, T, std::mt19937>(counter);
        test_performance<SAMPLE_TYPE, T, std::mt19937_64>(counter);
        test_performance<SAMPLE_TYPE, T, PCG>(counter);
}

template <SampleType SAMPLE_TYPE, typename Counter>
void test_performance(const Counter& counter)
{
        test_performance<SAMPLE_TYPE, float>(counter);
        test_performance<SAMPLE_TYPE, double>(counter);
        test_performance<SAMPLE_TYPE, long double>(counter);
}

void test(progress::Ratio* const progress)
{
        constexpr int COUNT = 6 * 3 * 3 * 2;
        int i = -1;
        const auto counter = [&]
        {
                progress->set(++i, COUNT);
        };
        test_performance<SampleType::ON_SPHERE>(counter);
        test_performance<SampleType::IN_SPHERE>(counter);
}

TEST_PERFORMANCE("Uniform Sphere Samples", test)
}
}
