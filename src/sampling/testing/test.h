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

#pragma once

#include "angle_distribution.h"
#include "functions.h"
#include "sphere_distribution.h"

#include <src/com/benchmark.h>
#include <src/com/chrono.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/random/pcg.h>
#include <src/com/string/str.h>
#include <src/com/thread.h>
#include <src/numerical/vector.h>
#include <src/progress/progress.h>

#include <cmath>
#include <cstddef>
#include <future>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

namespace ns::sampling::testing
{
template <std::size_t N, typename T, typename RandomVector>
void test_unit(
        const std::string_view description,
        const long long count,
        const RandomVector& random_vector,
        progress::Ratio* const progress)
{
        constexpr unsigned INDENT = 2;

        progress->set(0);

        {
                std::string s = "test unit length";
                if (!description.empty())
                {
                        s += ", " + printable_characters(description);
                }
                s += ", count " + to_string_digit_groups(count);
                LOG(add_indent(s, INDENT));
        }

        const int thread_count = hardware_concurrency();
        const long long count_per_thread = (count + thread_count - 1) / thread_count;
        const double count_per_thread_reciprocal = 1.0 / count_per_thread;

        const auto f = [&]()
        {
                PCG engine;
                for (long long i = 0; i < count_per_thread; ++i)
                {
                        if ((i & 0xfff) == 0xfff)
                        {
                                progress->set(i * count_per_thread_reciprocal);
                        }
                        const numerical::Vector<N, T> v = random_vector(engine);
                        if (!(v.is_unit()))
                        {
                                error("Vector " + to_string(v) + " is not unit " + to_string(v.norm()));
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
void test_distribution_angle(
        const std::string_view description,
        const long long count_per_bucket,
        const numerical::Vector<N, T>& normal,
        const RandomVector& random_vector,
        const PDF& pdf,
        progress::Ratio* const progress)
{
        constexpr unsigned INDENT = 2;

        progress->set(0);

        AngleDistribution<N, T> buckets;

        const long long count = [&]
        {
                const double c = round_distribution_count(buckets.distribution_count(count_per_bucket));
                return (c <= 1e9) ? c : 0;
        }();

        if (count <= 0)
        {
                return;
        }

        {
                std::string s = "test angle distribution";
                if (!description.empty())
                {
                        s += ", " + printable_characters(description);
                }
                s += ", count " + to_string_digit_groups(count);
                LOG(add_indent(s, INDENT));
        }

        buckets.compute_distribution(count, normal, random_vector, progress);
        // LOG(add_indent(buckets.histogram(pdf), 2 * INDENT));
        buckets.compare_with_pdf(pdf);
}

template <std::size_t N, typename T, typename RandomVector, typename PDF>
void test_distribution_surface(
        const std::string_view description,
        const long long count_per_bucket,
        const RandomVector& random_vector,
        const PDF& pdf,
        progress::Ratio* const progress)
{
        constexpr unsigned INDENT = 2;

        progress->set(0);

        const SphereDistribution<N, T> buckets(progress);

        const long long count = round_distribution_count(buckets.distribution_count(count_per_bucket));

        if (count <= 0)
        {
                return;
        }

        {
                std::string s = "test surface distribution";
                if (!description.empty())
                {
                        s += ", " + printable_characters(description);
                }
                s += ", buckets " + to_string_digit_groups(buckets.bucket_count());
                s += ", count " + to_string_digit_groups(count);
                LOG(add_indent(s, INDENT));
        }

        buckets.check_distribution(count, random_vector, pdf, progress);
}

template <long long COUNT, typename RandomVector>
long long test_performance(const RandomVector& random_vector)
{
        PCG engine;
        const Clock::time_point start_time = Clock::now();
        for (long long i = 0; i < COUNT; ++i)
        {
                do_not_optimize(random_vector(engine));
        }
        return std::llround(COUNT / duration_from(start_time));
}

template <long long COUNT, typename RandomVector>
void test_performance(
        const std::string_view description,
        const RandomVector& random_vector,
        progress::Ratio* const progress)
{
        constexpr unsigned INDENT = 2;

        progress->set(0);

        const long long performance = test_performance<COUNT>(random_vector);

        std::string s = to_string_digit_groups(performance) + " o/s";
        if (!description.empty())
        {
                s += ", " + printable_characters(description);
        }
        s += ", count " + to_string_digit_groups(COUNT);
        LOG(add_indent(s, INDENT));
}
}
