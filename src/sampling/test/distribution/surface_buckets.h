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

#include "surface_bucket.h"

#include "../../sphere_uniform.h"

#include <src/com/error.h>
#include <src/com/print.h>
#include <src/geometry/shapes/sphere_area.h>
#include <src/geometry/shapes/sphere_create.h>
#include <src/geometry/spatial/object_tree.h>
#include <src/numerical/vec.h>

#include <cmath>
#include <memory>
#include <optional>
#include <sstream>
#include <vector>

namespace ns::sampling::test
{
template <std::size_t N, typename T>
class SurfaceBuckets final
{
        static constexpr int TREE_MIN_OBJECTS_PER_BOX = 5;

        static int tree_max_depth()
        {
                static_assert(N >= 3);

                switch (N)
                {
                case 3:
                        return 10;
                case 4:
                        return 8;
                case 5:
                        return 6;
                case 6:
                        return 5;
                default:
                        // Сумма геометрической прогрессии s = (pow(r, n) - 1) / (r - 1).
                        // Для s и r найти n = log(s * (r - 1) + 1) / log(r).
                        double s = 1e9;
                        double r = std::pow(2, N);
                        double n = std::log(s * (r - 1) + 1) / std::log(r);
                        return std::max(2.0, std::floor(n));
                }
        }

        //

        static constexpr unsigned BUCKET_MIN_COUNT = 100 * (1 << N);

        std::unique_ptr<std::vector<Vector<N, T>>> m_vertices;
        std::vector<std::array<int, N>> m_facets;
        std::vector<Bucket<N, T>> m_buckets;

        long long m_missed_intersection_count = 0;
        long long m_intersection_count = 0;

        long long buckets_sample_count() const
        {
                long long s = 0;
                for (const Bucket<N, T>& bucket : m_buckets)
                {
                        s += bucket.sample_count();
                }
                return s;
        }

        long long buckets_uniform_count() const
        {
                long long s = 0;
                for (const Bucket<N, T>& bucket : m_buckets)
                {
                        s += bucket.uniform_count();
                }
                return s;
        }

        void check_bucket_intersection() const
        {
                const long long sample_count = m_missed_intersection_count + m_intersection_count;
                if (sample_count < 1'000'000)
                {
                        error("Too few samples " + to_string(sample_count));
                }
                const long long max_missed_count = std::ceil(sample_count * 1e-6);
                if (m_missed_intersection_count >= max_missed_count)
                {
                        std::ostringstream oss;
                        oss << "Too many missed intersections" << '\n';
                        oss << "missed intersections = " << m_missed_intersection_count << '\n';
                        oss << "all samples = " << sample_count << '\n';
                        oss << "missed/all = " << (double(m_missed_intersection_count) / sample_count);
                        error(oss.str());
                }
        }

        void check_bucket_sizes() const
        {
                ASSERT(!m_buckets.empty());
                long long min = limits<long long>::max();
                long long max = limits<long long>::lowest();
                for (const Bucket<N, T>& bucket : m_buckets)
                {
                        min = std::min(min, bucket.uniform_count());
                        max = std::max(max, bucket.uniform_count());
                }
                long long maximum_max_min_ratio = N < 5 ? 3 : 10;
                if (max > 0 && min > 0 && max < maximum_max_min_ratio * min)
                {
                        return;
                }
                std::ostringstream oss;
                oss << "Buckets max/min is too large" << '\n';
                oss << "max = " << max << '\n';
                oss << "min = " << min << '\n';
                oss << "max/min = " << (T(max) / min);
                error(oss.str());
        }

public:
        std::size_t bucket_count() const
        {
                return m_buckets.size();
        }

        long long distribution_count(const long long uniform_min_count_per_bucket) const
        {
                const double count = uniform_min_count_per_bucket * bucket_count();
                const double round_to = std::pow(10, std::round(std::log10(count)) - 2);
                return std::ceil(count / round_to) * round_to;
        }

        SurfaceBuckets()
        {
                m_vertices = std::make_unique<std::vector<Vector<N, T>>>();

                geometry::create_sphere(BUCKET_MIN_COUNT, m_vertices.get(), &m_facets);

                m_buckets.reserve(m_facets.size());
                for (const std::array<int, N>& vertex_indices : m_facets)
                {
                        m_buckets.emplace_back(*m_vertices, vertex_indices);
                }

                ASSERT(m_buckets.size() >= BUCKET_MIN_COUNT);
        }

        template <typename RandomEngine, typename RandomVector, typename PDF>
        void compute(
                RandomEngine& random_engine,
                const long long ray_count,
                const RandomVector& random_vector,
                const PDF& pdf)
        {
                std::optional<geometry::ObjectTree<Bucket<N, T>>> tree;
                {
                        ProgressRatio progress(nullptr);
                        tree.emplace(m_buckets, tree_max_depth(), TREE_MIN_OBJECTS_PER_BOX, &progress);
                }

                for (Bucket<N, T>& bucket : m_buckets)
                {
                        bucket.clear();
                }

                m_missed_intersection_count = 0;
                m_intersection_count = 0;

                for (long long i = 0; i < ray_count; ++i)
                {
                        const Ray<N, T> ray(Vector<N, T>(0), random_vector(random_engine));

                        const std::optional<T> root_distance = tree->intersect_root(ray);
                        ASSERT(root_distance && *root_distance == 0);

                        const std::optional<std::tuple<T, const Bucket<N, T>*>> v =
                                tree->intersect(ray, *root_distance);
                        if (!v)
                        {
                                ++m_missed_intersection_count;
                                continue;
                        }
                        ++m_intersection_count;

                        Bucket<N, T>& bucket = *const_cast<Bucket<N, T>*>(std::get<1>(*v));
                        bucket.add_sample();
                }

                const long long uniform_ray_count = 4 * ray_count;
                for (long long i = 0; i < uniform_ray_count; ++i)
                {
                        const Ray<N, T> ray(Vector<N, T>(0), uniform_on_sphere<N, T>(random_engine));

                        const std::optional<T> root_distance = tree->intersect_root(ray);
                        ASSERT(root_distance && *root_distance == 0);

                        const std::optional<std::tuple<T, const Bucket<N, T>*>> v =
                                tree->intersect(ray, *root_distance);
                        if (!v)
                        {
                                ++m_missed_intersection_count;
                                continue;
                        }
                        ++m_intersection_count;

                        Bucket<N, T>& bucket = *const_cast<Bucket<N, T>*>(std::get<1>(*v));
                        if ((m_intersection_count & 0b11) == 0b11)
                        {
                                bucket.add_pdf(pdf(ray.dir()));
                        }
                        bucket.add_uniform();
                }
        }

        void merge(const SurfaceBuckets<N, T>& other)
        {
                ASSERT(*m_vertices == *other.m_vertices);
                ASSERT(m_facets == other.m_facets);
                ASSERT(m_buckets.size() == other.m_buckets.size());

                for (std::size_t i = 0; i < m_buckets.size(); ++i)
                {
                        m_buckets[i].merge(other.m_buckets[i]);
                }

                m_intersection_count += other.m_intersection_count;
                m_missed_intersection_count += other.m_missed_intersection_count;
        }

        void compare() const
        {
                check_bucket_intersection();
                check_bucket_sizes();

                constexpr T UNIFORM_DENSITY = T(1) / geometry::sphere_area(N);

                const long long sample_count = buckets_sample_count();
                const long long uniform_count = buckets_uniform_count();

                T sum_sampled = 0;
                T sum_expected = 0;
                T sum_error = 0;

                for (const Bucket<N, T>& bucket : m_buckets)
                {
                        T bucket_area = bucket.area(uniform_count);
                        T sampled_distribution = T(bucket.sample_count()) / sample_count;
                        T sampled_density = sampled_distribution / bucket_area;
                        T expected_density = bucket.pdf();
                        T expected_distribution = expected_density * bucket_area;

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

                        T relative_error = std::abs(sampled_density - expected_density)
                                           / std::max(sampled_density, expected_density);

                        if (relative_error <= T(0.1))
                        {
                                continue;
                        }

                        std::ostringstream oss;
                        oss << "sampled distribution = " << sampled_distribution << '\n';
                        oss << "expected distribution = " << expected_distribution << '\n';
                        oss << "sampled density = " << sampled_density << '\n';
                        oss << "expected density = " << expected_density << '\n';
                        oss << "bucket area = " << bucket_area << '\n';
                        oss << "bucket sample count = " << bucket.sample_count() << '\n';
                        oss << "bucket uniform count = " << bucket.uniform_count() << '\n';
                        oss << "sample count = " << sample_count << '\n';
                        oss << "uniform count = " << uniform_count;
                        error(oss.str());
                }

                ASSERT(std::abs(sum_sampled - 1) < T(0.01));

                if (!(std::abs(sum_expected - 1) < T(0.01)))
                {
                        error("PDF integral " + to_string(sum_expected) + " is not equal to 1");
                }

                if (!(sum_error < T(0.01)))
                {
                        error("Absolute error " + to_string(sum_error));
                }
        }
};
}
