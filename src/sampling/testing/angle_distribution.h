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

#include <src/com/constant.h>
#include <src/com/error.h>
#include <src/com/print.h>
#include <src/com/random/engine.h>
#include <src/com/thread.h>
#include <src/com/type/limit.h>
#include <src/geometry/shapes/sphere_area.h>
#include <src/numerical/integrate.h>
#include <src/numerical/vec.h>
#include <src/progress/progress.h>

#include <algorithm>
#include <cmath>
#include <future>
#include <iomanip>
#include <sstream>
#include <vector>

namespace ns::sampling::testing
{
template <std::size_t N, typename T>
class AngleDistribution
{
        static constexpr int BUCKET_COUNT = 90;
        static constexpr T BUCKET_SIZE = PI<T> / BUCKET_COUNT;
        static constexpr T BUCKETS_PER_RADIAN = BUCKET_COUNT / PI<T>;

        static T to_degrees(T angle)
        {
                return angle * (180 / PI<T>);
        }

        struct Distribution
        {
                T angle_from;
                T angle_to;
                T distribution;
        };

        template <typename PDF>
        static T mean_pdf(const Distribution& d, const PDF& pdf)
        {
                static constexpr int COUNT = 100;
                const T integral = numerical::integrate<T>(pdf, d.angle_from, d.angle_to, COUNT);
                return integral / (d.angle_to - d.angle_from);
        }

        static void check_pdf_and_distribution(const T& pdf, const T& distribution)
        {
                if (!(pdf >= 0))
                {
                        error("PDF " + to_string(pdf, 5) + " is not positive and not zero");
                }
                if (!(distribution >= 0))
                {
                        error("Distribution " + to_string(distribution, 5) + " is not positive and not zero");
                }
        }

        template <typename RandomEngine, typename RandomVector>
        static int sample_bucket(
                RandomEngine& random_engine,
                const Vector<N, T>& normal,
                const RandomVector& random_vector)
        {
                Vector<N, T> v = random_vector(random_engine).normalized();
                T cosine = dot(v, normal);
                cosine = std::clamp(cosine, T(-1), T(1));
                T angle = std::acos(cosine);
                int bucket = angle * BUCKETS_PER_RADIAN;
                return std::clamp(bucket, 0, BUCKET_COUNT - 1);
        }

        template <typename RandomEngine, typename RandomVector>
        static std::vector<long long> compute_buckets(
                const long long count,
                const Vector<N, T>& normal,
                const RandomVector& random_vector,
                ProgressRatio* progress)
        {
                const int thread_count = hardware_concurrency();
                const long long count_per_thread = (count + thread_count - 1) / thread_count;
                const double count_per_thread_reciprocal = 1.0 / count_per_thread;

                const auto f = [&](std::vector<long long>* buckets)
                {
                        ASSERT(buckets->size() == BUCKET_COUNT);
                        RandomEngine random_engine = create_engine<RandomEngine>();
                        for (long long i = 0; i < count_per_thread; ++i)
                        {
                                if ((i & 0xfff) == 0xfff)
                                {
                                        progress->set(i * count_per_thread_reciprocal);
                                }
                                int bucket = sample_bucket(random_engine, normal, random_vector);
                                ++(*buckets)[bucket];
                        }
                };

                std::vector<std::vector<long long>> thread_buckets(thread_count);
                for (std::vector<long long>& buckets : thread_buckets)
                {
                        buckets.resize(BUCKET_COUNT, 0);
                }

                {
                        std::vector<std::future<void>> futures;
                        std::vector<std::thread> threads;
                        for (std::vector<long long>& buckets : thread_buckets)
                        {
                                std::packaged_task<void(std::vector<long long>*)> task(f);
                                futures.emplace_back(task.get_future());
                                threads.emplace_back(std::move(task), &buckets);
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

                std::vector<long long> result(BUCKET_COUNT, 0);
                for (const std::vector<long long>& buckets : thread_buckets)
                {
                        ASSERT(buckets.size() == BUCKET_COUNT);
                        for (std::size_t i = 0; i < BUCKET_COUNT; ++i)
                        {
                                result[i] += buckets[i];
                        }
                }
                return result;
        }

        std::vector<Distribution> distribution_;

public:
        static double distribution_count(const long long uniform_min_count_per_bucket)
        {
                const double bucket_size = BUCKET_SIZE;
                const double s_all = geometry::sphere_relative_area<N, long double>(0, PI<long double>);
                const double s_bucket = geometry::sphere_relative_area<N, long double>(0, bucket_size);
                const double count = s_all / s_bucket * uniform_min_count_per_bucket;
                const double round_to = std::pow(10, std::round(std::log10(count)) - 2);
                const double rounded_count = std::ceil(count / round_to) * round_to;
                return rounded_count;
        }

        template <typename RandomEngine, typename RandomVector>
        void compute_distribution(
                const long long count,
                const Vector<N, T>& normal,
                const RandomVector& random_vector,
                ProgressRatio* progress)
        {
                const std::vector<long long> buckets =
                        compute_buckets<RandomEngine>(count, normal, random_vector, progress);
                ASSERT(buckets.size() == BUCKET_COUNT);

                distribution_.clear();

                std::vector<T> distribution_values;
                distribution_values.reserve(buckets.size());

                const long double sphere_k =
                        geometry::sphere_area<N>() / geometry::sphere_relative_area<N, long double>(0, PI<T>);

                long long cnt = 0;
                for (unsigned bucket = 0; bucket < buckets.size(); ++bucket)
                {
                        Distribution& d = distribution_.emplace_back();

                        cnt += buckets[bucket];

                        d.angle_from = bucket * BUCKET_SIZE;
                        d.angle_to = (bucket + 1) * BUCKET_SIZE;

                        long double bucket_area =
                                sphere_k * geometry::sphere_relative_area<N, long double>(d.angle_from, d.angle_to);
                        d.distribution = buckets[bucket] / bucket_area;

                        distribution_values.push_back(d.distribution);
                }

                for (Distribution& d : distribution_)
                {
                        d.distribution /= cnt;
                }

                ASSERT(std::is_sorted(
                        distribution_.begin(), distribution_.end(),
                        [](const Distribution& d1, const Distribution& d2)
                        {
                                return d1.angle_from < d2.angle_from;
                        }));
        }

        template <typename PDF>
        std::string histogram(const PDF& pdf) const
        {
                constexpr int BAR_SIZE = 100;
                constexpr int DIVISION_SIZE = 10;

                if (distribution_.empty())
                {
                        error("There is no distribution");
                }

                T max = limits<T>::lowest();
                for (const Distribution& d : distribution_)
                {
                        max = std::max(max, d.distribution);
                }

                std::ostringstream oss;

                bool new_line = false;
                for (const Distribution& d : distribution_)
                {
                        const T distribution_value = d.distribution;
                        const T pdf_mean_value = mean_pdf(d, pdf);

                        check_pdf_and_distribution(pdf_mean_value, distribution_value);

                        if (new_line)
                        {
                                oss << '\n';
                        }
                        else
                        {
                                new_line = true;
                        }

                        oss << std::fixed << std::setprecision(1) << std::setw(5) << to_degrees(d.angle_from);
                        oss << ": ";
                        oss << std::scientific << std::setprecision(2) << distribution_value;
                        oss << " (" << pdf_mean_value << ")";
                        oss << " ";

                        const int count = std::round(distribution_value / max * BAR_SIZE);
                        for (int i = 0; i < count; i += DIVISION_SIZE)
                        {
                                oss << '+';
                                for (int j = i + 1; j < i + DIVISION_SIZE && j < count; ++j)
                                {
                                        oss << '*';
                                }
                        }
                }

                return oss.str();
        }

        template <typename PDF>
        void compare_with_pdf(const PDF& pdf) const
        {
                if (distribution_.empty())
                {
                        error("There is no distribution");
                }

                for (const Distribution& d : distribution_)
                {
                        const T distribution_value = d.distribution;
                        const T pdf_mean_value = mean_pdf(d, pdf);

                        check_pdf_and_distribution(pdf_mean_value, distribution_value);

                        if (pdf_mean_value == distribution_value)
                        {
                                continue;
                        }

                        const T discrepancy_abs = std::abs(pdf_mean_value - distribution_value);
                        if (discrepancy_abs <= T(0.05))
                        {
                                continue;
                        }

                        const T discrepancy_rel = discrepancy_abs / std::max(pdf_mean_value, distribution_value);
                        if (discrepancy_rel <= T(0.05))
                        {
                                continue;
                        }

                        error("Angle interval = [" + to_string(to_degrees(d.angle_from), 5) + ", "
                              + to_string(to_degrees(d.angle_to), 5) + "], distribution = "
                              + to_string(distribution_value, 5) + ", PDF = " + to_string(pdf_mean_value, 5));
                }
        }
};
}
