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

#include "angle_distribution.h"
#include "sphere_distribution.h"

#include <src/com/benchmark.h>
#include <src/com/chrono.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/random/engine.h>
#include <src/com/string/ascii.h>
#include <src/com/thread.h>
#include <src/progress/progress.h>

#include <algorithm>
#include <cmath>
#include <future>
#include <thread>
#include <vector>

namespace ns::sampling::testing
{
namespace test_implementation
{
inline void add_description(
        std::string* const message,
        const std::string_view& separator,
        const std::string& description)
{
        if (description.empty())
        {
                return;
        }
        (*message) += separator;
        for (char c : description)
        {
                (*message) += ascii::is_print(c) ? c : ' ';
        }
}

inline void log(const std::string& message, const bool add_indent = false)
{
        constexpr unsigned INDENT_SIZE = 2;
        const unsigned indent_size = (add_indent ? 2 : 1) * INDENT_SIZE;
        const std::string indent(indent_size, ' ');
        std::string s;
        s.reserve(indent_size + message.size());
        s += indent;
        for (const char c : message)
        {
                s += c;
                if (c == '\n')
                {
                        s += indent;
                }
        }
        LOG(s);
}
}

template <std::size_t N, typename T, typename RandomEngine, typename RandomVector>
void test_unit(
        const std::string& description,
        const long long count,
        const RandomVector& random_vector,
        ProgressRatio* const progress)
{
        namespace impl = test_implementation;

        progress->set(0);

        {
                std::string s = "test unit length";
                impl::add_description(&s, ", ", description);
                s += ", count " + to_string_digit_groups(count);
                impl::log(s);
        }

        const int thread_count = hardware_concurrency();
        const long long count_per_thread = (count + thread_count - 1) / thread_count;
        const double count_per_thread_reciprocal = 1.0 / count_per_thread;

        const auto f = [&]()
        {
                RandomEngine random_engine = create_engine<RandomEngine>();
                for (long long i = 0; i < count_per_thread; ++i)
                {
                        if ((i & 0xfff) == 0xfff)
                        {
                                progress->set(i * count_per_thread_reciprocal);
                        }
                        Vector<N, T> v = random_vector(random_engine);
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

template <std::size_t N, typename T, typename RandomEngine, typename RandomVector, typename PDF>
void test_distribution_angle(
        const std::string& description,
        const long long count_per_bucket,
        const Vector<N, T>& normal,
        const RandomVector& random_vector,
        const PDF& pdf,
        ProgressRatio* const progress)
{
        namespace impl = test_implementation;

        progress->set(0);

        AngleDistribution<N, T> buckets;

        const long long count = [&]
        {
                const double c = buckets.distribution_count(count_per_bucket);
                return (c <= 1e9) ? c : 0;
        }();
        if (count <= 0)
        {
                return;
        }

        {
                std::string s = "test angle distribution";
                impl::add_description(&s, ", ", description);
                s += ", count " + to_string_digit_groups(count);
                impl::log(s);
        }

        buckets.template compute_distribution<RandomEngine>(count, normal, random_vector, progress);
        // impl::log(buckets.histogram(pdf), true /*add_indent*/);
        buckets.compare_with_pdf(pdf);
}

template <std::size_t N, typename T, typename RandomEngine, typename RandomVector, typename PDF>
void test_distribution_surface(
        const std::string& description,
        const long long count_per_bucket,
        const RandomVector& random_vector,
        const PDF& pdf,
        ProgressRatio* const progress)
{
        namespace impl = test_implementation;

        progress->set(0);

        SphereDistribution<N, T> buckets(progress);

        const long long count = buckets.distribution_count(count_per_bucket);
        if (count <= 0)
        {
                return;
        }

        {
                std::string s = "test surface distribution";
                impl::add_description(&s, ", ", description);
                s += ", buckets " + to_string_digit_groups(buckets.bucket_count());
                s += ", count " + to_string_digit_groups(count);
                impl::log(s);
        }

        buckets.template check_distribution<RandomEngine>(count, random_vector, pdf, progress);
}

template <long long COUNT, typename RandomEngine, typename RandomVector>
long long test_performance(const RandomVector& random_vector)
{
        RandomEngine random_engine = create_engine<RandomEngine>();
        const Clock::time_point start_time = Clock::now();
        for (long long i = 0; i < COUNT; ++i)
        {
                do_not_optimize(random_vector(random_engine));
        }
        return std::llround(COUNT / duration_from(start_time));
}

template <long long COUNT, typename RandomEngine, typename RandomVector>
void test_performance(const std::string& description, const RandomVector& random_vector, ProgressRatio* const progress)
{
        namespace impl = test_implementation;

        progress->set(0);

        const long long performance = test_performance<COUNT, RandomEngine>(random_vector);

        std::string s = to_string_digit_groups(performance) + " o/s";
        impl::add_description(&s, ", ", description);
        s += ", count " + to_string_digit_groups(COUNT);
        impl::log(s);
}
}
