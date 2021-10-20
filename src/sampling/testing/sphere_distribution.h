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

#include "sphere_bucket.h"
#include "sphere_facet.h"
#include "sphere_intersection.h"

#include "../sphere_uniform.h"

#include <src/com/error.h>
#include <src/com/print.h>
#include <src/com/thread.h>
#include <src/geometry/shapes/sphere_area.h>
#include <src/geometry/shapes/sphere_create.h>
#include <src/geometry/spatial/object_tree.h>
#include <src/numerical/vec.h>
#include <src/progress/progress.h>

#include <array>
#include <cmath>
#include <future>
#include <sstream>
#include <vector>

namespace ns::sampling::testing
{
template <std::size_t N, typename T>
class SphereDistribution final
{
        static constexpr int TREE_MIN_OBJECTS_PER_BOX = 5;

        //

        static constexpr unsigned BUCKET_MIN_COUNT = 100 * (1 << N);

        struct Sphere final
        {
                std::vector<Vector<N, T>> vertices;
                std::vector<SphereFacet<N, T>> facets;

                Sphere()
                {
                        std::vector<std::array<int, N>> sphere_facets;
                        geometry::create_sphere(BUCKET_MIN_COUNT, &vertices, &sphere_facets);
                        ASSERT(sphere_facets.size() >= BUCKET_MIN_COUNT);

                        facets.reserve(sphere_facets.size());
                        for (const std::array<int, N>& vertex_indices : sphere_facets)
                        {
                                facets.emplace_back(&vertices, vertex_indices);
                        }
                }

                Sphere(const Sphere&) = delete;
                Sphere(Sphere&&) = delete;
                Sphere& operator=(const Sphere&) = delete;
                Sphere& operator=(Sphere&&) = delete;
        };

        //

        Sphere sphere_;
        geometry::ObjectTree<SphereFacet<N, T>> tree_;

        //

        template <typename RandomEngine, typename RandomVector, typename PDF>
        std::vector<SphereBucket<N, T>> compute_buckets(
                const long long count,
                const RandomVector& random_vector,
                const PDF& pdf,
                ProgressRatio* progress) const
        {
                const int thread_count = hardware_concurrency();
                const long long count_per_thread = (count + thread_count - 1) / thread_count;
                const double count_per_thread_reciprocal = 1.0 / count_per_thread;

                progress->set(0);

                const auto f = [&](std::vector<SphereBucket<N, T>>* buckets) -> std::array<long long, 2>
                {
                        ASSERT(buckets->size() == sphere_.facets.size());
                        SphereIntersection<N, T, RandomEngine> facet_finder(&tree_, &sphere_.facets);
                        for (long long i = 0; i < count_per_thread; ++i)
                        {
                                if ((i & 0xfff) == 0xfff)
                                {
                                        progress->set(i * count_per_thread_reciprocal);
                                }
                                {
                                        const auto [index, dir] = facet_finder.find(random_vector);
                                        (*buckets)[index].add_sample();
                                }
                                {
                                        const auto [index, dir] =
                                                facet_finder.find(uniform_on_sphere<N, T, RandomEngine>);
                                        (*buckets)[index].add_pdf(pdf(dir));
                                        (*buckets)[index].add_uniform();
                                }
                                for (int j = 0; j < 3; ++j)
                                {
                                        const auto [index, dir] =
                                                facet_finder.find(uniform_on_sphere<N, T, RandomEngine>);
                                        (*buckets)[index].add_uniform();
                                }
                        }
                        return {facet_finder.intersection_count(), facet_finder.missed_intersection_count()};
                };

                std::vector<std::vector<SphereBucket<N, T>>> thread_buckets(thread_count);
                for (std::vector<SphereBucket<N, T>>& buckets : thread_buckets)
                {
                        buckets.resize(sphere_.facets.size());
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

                std::vector<SphereBucket<N, T>> result(sphere_.facets.size());
                for (const std::vector<SphereBucket<N, T>>& buckets : thread_buckets)
                {
                        ASSERT(buckets.size() == sphere_.facets.size());
                        for (std::size_t i = 0; i < sphere_.facets.size(); ++i)
                        {
                                result[i].merge(buckets[i]);
                        }
                }
                return result;
        }

public:
        explicit SphereDistribution(ProgressRatio* progress)
                : sphere_(), tree_(&sphere_.facets, TREE_MIN_OBJECTS_PER_BOX, progress)
        {
        }

        SphereDistribution(const SphereDistribution&) = delete;
        SphereDistribution(SphereDistribution&&) = delete;
        SphereDistribution& operator=(const SphereDistribution&) = delete;
        SphereDistribution& operator=(SphereDistribution&&) = delete;

        std::size_t bucket_count() const
        {
                return sphere_.facets.size();
        }

        long long distribution_count(const long long uniform_min_count_per_bucket) const
        {
                const double count = uniform_min_count_per_bucket * bucket_count();
                const double round_to = std::pow(10, std::round(std::log10(count)) - 2);
                return std::ceil(count / round_to) * round_to;
        }

        template <typename RandomEngine, typename RandomVector, typename PDF>
        void check_distribution(
                const long long count,
                const RandomVector& random_vector,
                const PDF& pdf,
                ProgressRatio* progress) const
        {
                const std::vector<SphereBucket<N, T>> buckets =
                        compute_buckets<RandomEngine>(count, random_vector, pdf, progress);

                check_bucket_sizes(buckets);

                static constexpr T UNIFORM_DENSITY = 1 / geometry::SPHERE_AREA<N, long double>;

                const long long sample_count = buckets_sample_count(buckets);
                const long long uniform_count = buckets_uniform_count(buckets);

                long double sum_sampled = 0;
                long double sum_expected = 0;
                long double sum_error = 0;

                ASSERT(buckets.size() == sphere_.facets.size());
                for (std::size_t i = 0; i < buckets.size(); ++i)
                {
                        const SphereBucket<N, T>& bucket = buckets[i];

                        const T bucket_area =
                                sphere_facet_area(sphere_.facets[i], bucket.uniform_count(), uniform_count);
                        const T sampled_distribution = T(bucket.sample_count()) / sample_count;
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

                        sum_sampled += sampled_distribution;
                        sum_expected += expected_distribution;
                        sum_error += std::abs(sampled_distribution - expected_distribution);

                        if (expected_density == sampled_density)
                        {
                                continue;
                        }

                        if (expected_density < UNIFORM_DENSITY / 2)
                        {
                                continue;
                        }

                        const T max_relative_error = expected_density < UNIFORM_DENSITY ? 0.2 : 0.1;

                        const T relative_error = std::abs(sampled_density - expected_density)
                                                 / std::max(sampled_density, expected_density);

                        if (relative_error <= max_relative_error)
                        {
                                continue;
                        }

                        const T uniform_distribution = T(bucket.uniform_count()) / uniform_count;
                        const T uniform_density = uniform_distribution / bucket_area;
                        const T bucket_relative_area = bucket_area / geometry::SPHERE_AREA<N, T>;

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
