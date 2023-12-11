/*
Copyright (C) 2017-2023 Topological Manifold

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

#include "sphere_bucket.h"
#include "sphere_intersection.h"
#include "sphere_mesh.h"

#include "../sphere_uniform.h"

#include <src/com/error.h>
#include <src/com/print.h>
#include <src/com/random/pcg.h>
#include <src/com/thread.h>
#include <src/geometry/shapes/sphere_area.h>
#include <src/geometry/shapes/sphere_simplex.h>
#include <src/numerical/vector.h>
#include <src/progress/progress.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <future>
#include <sstream>
#include <thread>
#include <vector>

namespace ns::sampling::testing
{
template <std::size_t N, typename T>
class SphereDistribution final
{
        static constexpr unsigned BUCKET_MIN_COUNT = 100 * (1 << N);

        SphereMesh<N, T> sphere_mesh_;

        [[nodiscard]] double sphere_facet_area(
                const unsigned facet_index,
                const long long uniform_count,
                const long long all_uniform_count) const
        {
                const double area = static_cast<double>(uniform_count) / all_uniform_count
                                    * geometry::shapes::SPHERE_AREA<N, double>;

                constexpr bool FUNCTION =
                        requires { geometry::shapes::sphere_simplex_area(sphere_mesh_.facet_vertices(facet_index)); };
                static_assert(FUNCTION || N >= 4);

                if constexpr (FUNCTION)
                {
                        const std::array<Vector<N, T>, N> vertices = sphere_mesh_.facet_vertices(facet_index);
                        const double geometry_area = geometry::shapes::sphere_simplex_area(vertices);
                        const double relative_error = std::abs(area - geometry_area) / std::max(geometry_area, area);

                        if (relative_error < 0.025)
                        {
                                return geometry_area;
                        }

                        std::ostringstream oss;
                        oss << "sphere area relative error = " << relative_error << '\n';
                        oss << "sphere area = " << area << '\n';
                        oss << "geometry sphere area = " << geometry_area << '\n';
                        oss << "uniform count = " << uniform_count << '\n';
                        oss << "all uniform count = " << all_uniform_count;
                        error(oss.str());
                }

                return area;
        }

        template <typename F>
        [[nodiscard]] std::vector<SphereBucket<N, T>> compute_buckets_threads(const unsigned thread_count, const F& f)
                const
        {
                std::vector<std::vector<SphereBucket<N, T>>> thread_buckets(thread_count);
                for (std::vector<SphereBucket<N, T>>& buckets : thread_buckets)
                {
                        buckets.resize(sphere_mesh_.facet_count());
                }

                {
                        std::vector<std::future<std::array<long long, 2>>> futures;

                        std::vector<std::thread> threads;
                        for (std::vector<SphereBucket<N, T>>& buckets : thread_buckets)
                        {
                                std::packaged_task<std::array<long long, 2>(std::vector<SphereBucket<N, T>>*)> task(f);
                                futures.emplace_back(task.get_future());
                                threads.emplace_back(std::move(task), &buckets);
                        }
                        for (std::thread& thread : threads)
                        {
                                thread.join();
                        }

                        long long intersection_count = 0;
                        long long missed_intersection_count = 0;
                        for (std::future<std::array<long long, 2>>& future : futures)
                        {
                                const auto [intersections, missed_intersections] = future.get();
                                intersection_count += intersections;
                                missed_intersection_count += missed_intersections;
                        }

                        check_sphere_intersections(intersection_count, missed_intersection_count);
                }

                std::vector<SphereBucket<N, T>> res(sphere_mesh_.facet_count());
                for (const std::vector<SphereBucket<N, T>>& buckets : thread_buckets)
                {
                        ASSERT(buckets.size() == res.size());
                        for (std::size_t i = 0; i < res.size(); ++i)
                        {
                                res[i].merge(buckets[i]);
                        }
                }
                return res;
        }

        template <typename RandomVector, typename PDF>
        [[nodiscard]] std::vector<SphereBucket<N, T>> compute_buckets(
                const long long count,
                const RandomVector& random_vector,
                const PDF& pdf,
                progress::Ratio* const progress) const
        {
                progress->set(0);

                constexpr long long GROUP_SIZE = 1 << 16;

                const int thread_count = hardware_concurrency();
                const long long count_per_thread = [&]
                {
                        const long long min_count_per_thread = (count + thread_count - 1) / thread_count;
                        const long long group_count = (min_count_per_thread + GROUP_SIZE - 1) / GROUP_SIZE;
                        return group_count * GROUP_SIZE;
                }();
                const long long all_count = count_per_thread * thread_count;
                const double all_count_reciprocal = 1.0 / all_count;

                ASSERT(all_count >= count);
                ASSERT(count_per_thread % GROUP_SIZE == 0);

                std::atomic<long long> counter = 0;

                const auto f = [&](std::vector<SphereBucket<N, T>>* const buckets)
                {
                        ASSERT(buckets->size() == sphere_mesh_.facet_count());

                        SphereIntersection<N, T> intersections(&sphere_mesh_);

                        PCG engine;

                        const auto random = [&]
                        {
                                return random_vector(engine);
                        };

                        const auto uniform = [&]
                        {
                                return uniform_on_sphere<N, T>(engine);
                        };

                        const auto add_data = [&]
                        {
                                {
                                        const auto [index, dir] = intersections.find(random);
                                        (*buckets)[index].add_sample();
                                }
                                {
                                        const auto [index, dir] = intersections.find(uniform);
                                        (*buckets)[index].add_pdf(pdf(dir));
                                        (*buckets)[index].add_uniform();
                                }
                                for (int i = 0; i < 3; ++i)
                                {
                                        const auto [index, dir] = intersections.find(uniform);
                                        (*buckets)[index].add_uniform();
                                }
                        };

                        for (long long i = 0; i < count_per_thread; i += GROUP_SIZE)
                        {
                                for (long long j = 0; j < GROUP_SIZE; ++j)
                                {
                                        add_data();
                                }
                                counter += GROUP_SIZE;
                                progress->set(counter * all_count_reciprocal);
                        }

                        std::array<long long, 2> res;
                        res[0] = intersections.intersection_count();
                        res[1] = intersections.missed_intersection_count();
                        return res;
                };

                return compute_buckets_threads(thread_count, f);
        }

        void check_bucket_distribution(
                const long long sample_count,
                const long long uniform_count,
                const std::vector<SphereBucket<N, T>> buckets,
                const unsigned facet_index,
                long double* const sum_sampled,
                long double* const sum_expected,
                long double* const sum_error) const
        {
                static constexpr T UNIFORM_DENSITY = 1 / geometry::shapes::SPHERE_AREA<N, long double>;

                const SphereBucket<N, T>& bucket = buckets[facet_index];

                const T bucket_area = sphere_facet_area(facet_index, bucket.uniform_count(), uniform_count);
                const T sampled_distribution = static_cast<T>(bucket.sample_count()) / sample_count;
                const T sampled_density = sampled_distribution / bucket_area;
                const T expected_density = bucket.pdf();
                const T expected_distribution = expected_density * bucket_area;

                ASSERT(sampled_density >= 0);
                ASSERT(sampled_distribution >= 0);
                if (!(expected_density >= 0))
                {
                        error("PDF " + to_string(expected_density) + " is not positive and not zero");
                }
                ASSERT(expected_distribution >= 0);

                *sum_sampled += sampled_distribution;
                *sum_expected += expected_distribution;
                *sum_error += std::abs(sampled_distribution - expected_distribution);

                if (expected_density == sampled_density)
                {
                        return;
                }

                if (expected_density < UNIFORM_DENSITY / 2)
                {
                        return;
                }

                const T max_relative_error = expected_density < UNIFORM_DENSITY ? 0.2 : 0.1;

                const T relative_error =
                        std::abs(sampled_density - expected_density) / std::max(sampled_density, expected_density);

                if (relative_error <= max_relative_error)
                {
                        return;
                }

                const T uniform_distribution = static_cast<T>(bucket.uniform_count()) / uniform_count;
                const T uniform_density = uniform_distribution / bucket_area;
                const T bucket_relative_area = bucket_area / geometry::shapes::SPHERE_AREA<N, T>;

                std::ostringstream oss;
                oss << "sampled distribution = " << sampled_distribution << '\n';
                oss << "expected distribution = " << expected_distribution << '\n';
                oss << "uniform distribution = " << uniform_distribution << '\n';
                oss << "sampled density = " << sampled_density << '\n';
                oss << "expected density = " << expected_density << '\n';
                oss << "uniform density = " << UNIFORM_DENSITY << '\n';
                oss << "uniform computed density = " << uniform_density << '\n';
                oss << "bucket area = " << bucket_area << '\n';
                oss << "bucket relative area = 1 / " << 1 / bucket_relative_area << '\n';
                oss << "bucket sample count = " << bucket.sample_count() << '\n';
                oss << "sample count = " << sample_count << '\n';
                oss << "bucket uniform count = " << bucket.uniform_count() << '\n';
                oss << "uniform count = " << uniform_count;
                error(oss.str());
        }

public:
        explicit SphereDistribution(progress::Ratio* const progress)
                : sphere_mesh_(BUCKET_MIN_COUNT, progress)
        {
        }

        [[nodiscard]] unsigned bucket_count() const
        {
                return sphere_mesh_.facet_count();
        }

        [[nodiscard]] double distribution_count(const long long uniform_min_count_per_bucket) const
        {
                const double count = uniform_min_count_per_bucket * bucket_count();
                return count;
        }

        template <typename RandomVector, typename PDF>
        void check_distribution(
                const long long count,
                const RandomVector& random_vector,
                const PDF& pdf,
                progress::Ratio* const progress) const
        {
                const std::vector<SphereBucket<N, T>> buckets = compute_buckets(count, random_vector, pdf, progress);

                check_bucket_sizes(buckets);

                const long long sample_count = buckets_sample_count(buckets);
                const long long uniform_count = buckets_uniform_count(buckets);

                long double sum_sampled = 0;
                long double sum_expected = 0;
                long double sum_error = 0;

                ASSERT(buckets.size() == sphere_mesh_.facet_count());
                for (std::size_t i = 0; i < buckets.size(); ++i)
                {
                        check_bucket_distribution(
                                sample_count, uniform_count, buckets, i, &sum_sampled, &sum_expected, &sum_error);
                }

                ASSERT(std::abs(sum_sampled - 1) < 0.01);

                if (!(std::abs(sum_expected - 1) < 0.01))
                {
                        error("PDF integral " + to_string(sum_expected) + " is not equal to 1");
                }

                if (!(sum_error < 0.01))
                {
                        error("Absolute error " + to_string(sum_error));
                }
        }
};
}
