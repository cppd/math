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

#include "test_sphere_distribution.h"

#include "sphere_buckets.h"

#include "../sphere.h"

#include <src/com/constant.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/names.h>
#include <src/com/random/engine.h>
#include <src/com/thread.h>
#include <src/com/time.h>
#include <src/com/type/name.h>

#include <cmath>
#include <future>
#include <random>
#include <string>
#include <thread>

namespace ns::random
{
namespace
{
template <typename T>
using RandomEngine = std::conditional_t<sizeof(T) <= 4, std::mt19937, std::mt19937_64>;

template <typename T>
T pdf_uniform(std::type_identity_t<T> angle)
{
        // ProbabilityDistribution[1, {x, 0, Pi},  Method -> "Normalize"]
        if (angle >= 0 && angle < (PI<T>))
        {
                return 1 / PI<T>;
        }
        return 0;
}

template <typename T>
T pdf_cosine(std::type_identity_t<T> angle)
{
        // ProbabilityDistribution[Cos[x], {x, 0, Pi/2}, Method -> "Normalize"]
        if (angle >= 0 && angle < (PI<T> / 2))
        {
                return std::cos(angle);
        }
        return 0;
}

template <typename T>
T pdf_power_cosine(std::type_identity_t<T> angle, std::type_identity_t<T> power)
{
        // Assuming[n >= 0,
        //   ProbabilityDistribution[Cos[x]^n, {x, 0, Pi/2},
        //   Method -> "Normalize"]]
        if (angle >= 0 && angle < (PI<T> / 2))
        {
                T norm = 2 / std::sqrt(PI<T>);
                norm *= std::exp(std::lgamma((2 + power) / 2) - std::lgamma((1 + power) / 2));
                T v = norm * std::pow(std::cos(angle), power);
                return v;
        }
        return 0;
}

template <std::size_t N, typename T, typename RandomVector>
void test_unit(const std::string& name, long long count, const RandomVector& random_vector)
{
        LOG(name + "\n  test unit in " + space_name(N) + ", " + to_string_digit_groups(count) + ", " + type_name<T>());

        const Vector<N, T> normal = []()
        {
                RandomEngine<T> random_engine = create_engine<RandomEngine<T>>();
                return random_on_sphere<N, T>(random_engine).normalized();
        }();

        const int thread_count = hardware_concurrency();
        const long long count_per_thread = (count + thread_count - 1) / thread_count;

        const auto f = [&]()
        {
                RandomEngine<T> random_engine = create_engine<RandomEngine<T>>();
                for (long long i = 0; i < count_per_thread; ++i)
                {
                        T v_norm = random_vector(random_engine, normal).norm();
                        if (!(v_norm >= T(0.999) && v_norm <= T(1.001)))
                        {
                                error(name + " vector is not unit " + to_string(v_norm));
                        }
                }
        };

        std::vector<std::future<void>> futures;
        std::vector<std::thread> threads;
        for (int i = 0; i < thread_count; ++i)
        {
                std::packaged_task<void()> task(f);
                futures.emplace_back(task.get_future());
                threads.emplace_back(std::move(task));
        }
        for (std::thread& thread : threads)
        {
                thread.join();
        }
        for (std::future<void>& future : futures)
        {
                future.get();
        }
}

template <std::size_t N, typename T, typename RandomVector, typename PDF>
void test_distribution(const std::string& name, long long count, const RandomVector& random_vector, const PDF& pdf)
{
        LOG(name + "\n  test distribution in " + space_name(N) + ", " + to_string_digit_groups(count) + ", "
            + type_name<T>());

        RandomEngine<T> random_engine = create_engine<RandomEngine<T>>();

        SphereBuckets<N, T> buckets;

        const Vector<N, T> normal = random_on_sphere<N, T>(random_engine).normalized();

        for (long long i = 0; i < count; ++i)
        {
                Vector<N, T> v = random_vector(random_engine, normal).normalized();
                T cosine = dot(v, normal);
                cosine = std::clamp(cosine, T(-1), T(1));
                buckets.add(std::acos(cosine));
        }

        buckets.compute_distribution();
        LOG(buckets.histogram());
        buckets.compare_with_pdf(pdf);
}

template <std::size_t N, typename T, typename RandomVector>
void test_performance(const std::string& name, long long count, const RandomVector& random_vector)
{
        LOG(name + "\n  test performance in " + space_name(N) + ", " + to_string_digit_groups(count) + ", "
            + type_name<T>());

        RandomEngine<T> random_engine = create_engine<RandomEngine<T>>();

        const Vector<N, T> normal = random_on_sphere<N, T>(random_engine);

        static Vector<N, T> sink;
        TimePoint start_time = time();
        for (long long i = 0; i < count; ++i)
        {
                sink = random_vector(random_engine, normal);
        }
        LOG("  " + to_string_digit_groups(std::lround(count / duration_from(start_time))) + " per second");
}

template <std::size_t N, typename T>
void test_uniform_on_sphere(long long unit_count, long long distribution_count, long long performance_count)
{
        const std::string NAME = "uniform";

        test_unit<N, T>(
                NAME, unit_count,
                [&](RandomEngine<T>& random_engine, const Vector<N, T>& /*normal*/)
                {
                        return random_on_sphere<N, T>(random_engine);
                });

        test_distribution<N, T>(
                NAME, distribution_count,
                [&](RandomEngine<T>& random_engine, const Vector<N, T>& /*normal*/)
                {
                        return random_on_sphere<N, T>(random_engine);
                },
                [](T angle)
                {
                        return pdf_uniform<T>(angle);
                });

        test_performance<N, T>(
                NAME, performance_count,
                [&](RandomEngine<T>& random_engine, const Vector<N, T>& /*normal*/)
                {
                        return random_on_sphere<N, T>(random_engine);
                });
}

template <std::size_t N, typename T>
void test_cosine_on_hemisphere(long long unit_count, long long distribution_count, long long performance_count)
{
        const std::string NAME = "cosine_weighted";

        test_unit<N, T>(
                NAME, unit_count,
                [&](RandomEngine<T>& random_engine, const Vector<N, T>& normal)
                {
                        return random_cosine_weighted_on_hemisphere(random_engine, normal);
                });

        test_distribution<N, T>(
                NAME, distribution_count,
                [&](RandomEngine<T>& random_engine, const Vector<N, T>& normal)
                {
                        return random_cosine_weighted_on_hemisphere(random_engine, normal);
                },
                [](T angle)
                {
                        return pdf_cosine<T>(angle);
                });

        test_performance<N, T>(
                NAME, performance_count,
                [&](RandomEngine<T>& random_engine, const Vector<N, T>& normal)
                {
                        return random_cosine_weighted_on_hemisphere(random_engine, normal);
                });
}

template <std::size_t N, typename T>
void test_power_cosine_on_hemisphere(long long unit_count, long long distribution_count, long long performance_count)
{
        const T POWER = []()
        {
                RandomEngine<T> random_engine = create_engine<RandomEngine<T>>();
                return std::uniform_real_distribution<T>(1, 100)(random_engine);
        }();

        const std::string NAME = "power_" + to_string_fixed(POWER, 1) + "_cosine_weighted";

        test_unit<N, T>(
                NAME, unit_count,
                [&](RandomEngine<T>& random_engine, const Vector<N, T>& normal)
                {
                        return random_power_cosine_weighted_on_hemisphere(random_engine, normal, POWER);
                });

        test_distribution<N, T>(
                NAME, distribution_count,
                [&](RandomEngine<T>& random_engine, const Vector<N, T>& normal)
                {
                        return random_power_cosine_weighted_on_hemisphere(random_engine, normal, POWER);
                },
                [&](T angle)
                {
                        return pdf_power_cosine<T>(angle, POWER);
                });

        test_performance<N, T>(
                NAME, performance_count,
                [&](RandomEngine<T>& random_engine, const Vector<N, T>& normal)
                {
                        return random_power_cosine_weighted_on_hemisphere(random_engine, normal, POWER);
                });
}

template <std::size_t N, typename T>
void test_distribution(long long distribution_count)
{
        constexpr int UNIT_COUNT = 10'000'000;
        constexpr int PERFORMANCE_COUNT = 10'000'000;

        test_uniform_on_sphere<N, T>(UNIT_COUNT, distribution_count, PERFORMANCE_COUNT);
        LOG("");
        test_cosine_on_hemisphere<N, T>(UNIT_COUNT, distribution_count, PERFORMANCE_COUNT);
        LOG("");
        if constexpr (N == 3)
        {
                test_power_cosine_on_hemisphere<N, T>(UNIT_COUNT, distribution_count, PERFORMANCE_COUNT);
                LOG("");
        }
}

template <typename T>
void test_distribution()
{
        test_distribution<3, T>(50'000'000);
        test_distribution<4, T>(100'000'000);
        test_distribution<5, T>(200'000'000);
        test_distribution<6, T>(300'000'000);
}
}

void test_sphere_distribution()
{
        test_distribution<float>();
        test_distribution<double>();
}
}
