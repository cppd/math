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

#pragma once

#include "angle_buckets.h"
#include "surface_buckets.h"

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/random/engine.h>
#include <src/com/thread.h>
#include <src/com/time.h>

#include <algorithm>
#include <cmath>
#include <future>
#include <thread>
#include <vector>

namespace ns::sampling::test
{
template <std::size_t N, typename T, typename RandomEngine, typename RandomVector>
void test_unit(const long long count, const RandomVector& random_vector)
{
        LOG("  test unit length, count " + to_string_digit_groups(count));

        const int thread_count = hardware_concurrency();
        const long long count_per_thread = (count + thread_count - 1) / thread_count;

        const auto f = [&]()
        {
                RandomEngine random_engine = create_engine<RandomEngine>();
                for (long long i = 0; i < count_per_thread; ++i)
                {
                        Vector<N, T> v = random_vector(random_engine);
                        T v_norm = v.norm();
                        if (!(v_norm >= T(0.999) && v_norm <= T(1.001)))
                        {
                                error("Vector " + to_string(v) + " is not unit " + to_string(v_norm));
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

template <std::size_t N, typename T, typename RandomEngine, typename RandomVector, typename PDF>
void test_distribution_angle(
        const std::string& description,
        const long long count_per_bucket,
        const Vector<N, T>& normal,
        const RandomVector& random_vector,
        const PDF& pdf)
{
        AngleBuckets<N, T> buckets;

        const long long count = buckets.distribution_count(count_per_bucket);

        if (count <= 0)
        {
                return;
        }

        LOG("  test angle distribution" + (description.empty() ? "" : ", " + description) + ", count "
            + to_string_digit_groups(count));

        const int thread_count = hardware_concurrency();
        const long long count_per_thread = (count + thread_count - 1) / thread_count;

        const auto f = [&count_per_thread, &normal, &random_vector]()
        {
                AngleBuckets<N, T> thread_buckets;
                RandomEngine random_engine = create_engine<RandomEngine>();
                thread_buckets.compute(random_engine, count_per_thread, normal, random_vector);
                return thread_buckets;
        };

        {
                std::vector<std::future<AngleBuckets<N, T>>> futures;
                std::vector<std::thread> threads;
                for (int i = 0; i < thread_count; ++i)
                {
                        std::packaged_task<AngleBuckets<N, T>()> task(f);
                        futures.emplace_back(task.get_future());
                        threads.emplace_back(std::move(task));
                }
                for (std::thread& thread : threads)
                {
                        thread.join();
                }
                for (std::future<AngleBuckets<N, T>>& future : futures)
                {
                        buckets.merge(future.get());
                }
        }

        buckets.compute_distribution();
        //LOG(buckets.histogram(pdf));
        buckets.compare_with_pdf(pdf);
}

template <std::size_t N, typename T, typename RandomEngine, typename RandomVector, typename PDF>
void test_distribution_surface(
        const std::string& description,
        const long long count_per_bucket,
        const RandomVector& random_vector,
        const PDF& pdf)
{
        SurfaceBuckets<N, T> buckets;

        const long long count = buckets.distribution_count(count_per_bucket);

        if (count <= 0)
        {
                return;
        }

        LOG("  test surface distribution" + (description.empty() ? "" : ", " + description) + ", buckets "
            + to_string_digit_groups(buckets.bucket_count()) + ", count " + to_string_digit_groups(count));

        const int thread_count = hardware_concurrency();
        const long long count_per_thread = (count + thread_count - 1) / thread_count;

        const auto f = [&count_per_thread, &random_vector, &pdf]()
        {
                SurfaceBuckets<N, T> thread_buckets;
                RandomEngine random_engine = create_engine<RandomEngine>();
                thread_buckets.compute(random_engine, count_per_thread, random_vector, pdf);
                return thread_buckets;
        };

        {
                std::vector<std::future<SurfaceBuckets<N, T>>> futures;
                std::vector<std::thread> threads;
                for (int i = 0; i < thread_count; ++i)
                {
                        std::packaged_task<SurfaceBuckets<N, T>()> task(f);
                        futures.emplace_back(task.get_future());
                        threads.emplace_back(std::move(task));
                }
                for (std::thread& thread : threads)
                {
                        thread.join();
                }
                for (std::future<SurfaceBuckets<N, T>>& future : futures)
                {
                        buckets.merge(future.get());
                }
        }

        buckets.compare();
}

template <std::size_t N, typename T, typename RandomEngine, typename RandomVector>
void test_performance(const long long count, const RandomVector& random_vector)
{
        LOG("  test performance, count " + to_string_digit_groups(count));

        RandomEngine random_engine = create_engine<RandomEngine>();

        static Vector<N, T> sink;
        const TimePoint start_time = time();
        for (long long i = 0; i < count; ++i)
        {
                sink = random_vector(random_engine);
        }
        LOG("  performance " + to_string_digit_groups(std::lround(count / duration_from(start_time))) + " per second");
}
}
