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

#include "mesh_facet.h"

#include <src/com/error.h>
#include <src/com/print.h>
#include <src/com/random/engine.h>
#include <src/geometry/shapes/sphere_create.h>
#include <src/geometry/shapes/sphere_simplex.h>
#include <src/geometry/shapes/sphere_surface.h>
#include <src/geometry/spatial/object_tree.h>
#include <src/numerical/vec.h>

#include <optional>
#include <random>
#include <sstream>
#include <vector>

namespace ns::sampling
{
namespace surface_buckets_implementation
{
constexpr int TREE_MIN_OBJECTS_PER_BOX = 5;

template <std::size_t N>
int tree_max_depth()
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
}

template <std::size_t N, typename T>
class SurfaceBuckets final
{
        template <typename PDF, typename RandomEngine>
        static T pdf_for_bucket(
                const std::array<Vector<N, T>, N>& vertices,
                const PDF& pdf,
                RandomEngine& random_engine)
        {
                std::uniform_real_distribution<T> urd(0, 1);
                constexpr int COUNT = 100;
                T bucket_pdf = 0;
                for (int i = 0; i < COUNT; ++i)
                {
                        Vector<N, T> v;
                        while (true)
                        {
                                v = Vector<N, T>(0);
                                T sum = 0;
                                for (unsigned n = 0; n < N - 1; ++n)
                                {
                                        T rnd = urd(random_engine);
                                        sum += rnd;
                                        v += vertices[n] * rnd;
                                }
                                if (sum < 1)
                                {
                                        v += vertices[N - 1] * (1 - sum);
                                        break;
                                }
                        }
                        bucket_pdf += pdf(v.normalized());
                }
                return bucket_pdf / COUNT;
        }

        struct Bucket final : public MeshFacet<N, T>
        {
                long long counter = 0;

                using MeshFacet<N, T>::MeshFacet;
        };

        static constexpr unsigned BUCKET_MIN_COUNT = 1000;

        std::vector<Vector<N, T>> m_vertices;
        std::vector<Bucket> m_buckets;

        std::optional<geometry::ObjectTree<Bucket>> m_tree;

public:
        std::size_t bucket_count() const
        {
                return m_buckets.size();
        }

        SurfaceBuckets()
        {
                namespace impl = surface_buckets_implementation;

                std::vector<std::array<int, N>> facets;

                geometry::create_sphere(BUCKET_MIN_COUNT, &m_vertices, &facets);

                m_buckets.reserve(facets.size());
                for (const std::array<int, N>& vertex_indices : facets)
                {
                        m_buckets.emplace_back(m_vertices, vertex_indices);
                }

                ProgressRatio progress(nullptr);

                m_tree.emplace(m_buckets, impl::tree_max_depth<N>(), impl::TREE_MIN_OBJECTS_PER_BOX, &progress);

                ASSERT(m_buckets.size() >= BUCKET_MIN_COUNT);
        }

        void merge(const SurfaceBuckets<N, T>& other)
        {
                ASSERT(m_buckets.size() == other.m_buckets.size());
                for (unsigned i = 0; i < m_buckets.size(); ++i)
                {
                        m_buckets[i].counter += other.m_buckets[i].counter;
                }
        }

        void add(const Vector<N, T>& direction)
        {
                Ray<N, T> ray(Vector<N, T>(1e-4), direction);

                std::optional<T> root_distance = m_tree->intersect_root(ray);
                ASSERT(root_distance && *root_distance == 0);

                std::optional<std::tuple<T, const Bucket*>> v = m_tree->intersect(ray, *root_distance);
                if (!v)
                {
                        //error("No intersection found for direction " + to_string(ray.dir()));
                        return;
                }

                ++const_cast<Bucket*>(std::get<1>(*v))->counter;
        }

        template <typename PDF>
        void compare_with_pdf(const PDF& pdf) const
        {
                constexpr T UNIFORM_DENSITY = T(1) / geometry::sphere_area(N);

                const long long sample_count = [&]()
                {
                        long long s = 0;
                        for (const Bucket& bucket : m_buckets)
                        {
                                s += bucket.counter;
                        }
                        return s;
                }();

                std::mt19937 random_engine = create_engine<std::mt19937>();

                T sum_sampled = 0;
                T sum_expected = 0;
                T sum_error = 0;

                for (const Bucket& bucket : m_buckets)
                {
                        std::array<Vector<N, T>, N> vertices = bucket.vertices();
                        T bucket_area = geometry::sphere_simplex_area(vertices);

                        T sampled_distribution = T(bucket.counter) / sample_count;
                        T sampled_density = sampled_distribution / bucket_area;
                        T expected_density = pdf_for_bucket(vertices, pdf, random_engine);
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

                        if (relative_error <= T(0.3))
                        {
                                continue;
                        }

                        std::ostringstream oss;
                        oss << "sampled distribution = " << sampled_distribution << '\n';
                        oss << "expected distribution = " << expected_distribution << '\n';
                        oss << "sampled density = " << sampled_density << '\n';
                        oss << "expected density = " << expected_density << '\n';
                        oss << "bucket area = " << bucket_area << '\n';
                        oss << "bucket counter = " << bucket.counter << '\n';
                        oss << "sample count = " << sample_count;
                        error(oss.str());
                }

                ASSERT(std::abs(sum_sampled - 1) < T(0.01));
                if (!(std::abs(sum_expected - 1) < T(0.02)))
                {
                        error("PDF integral " + to_string(sum_expected) + " is not equal to 1");
                }

                if (!(sum_error < T(0.03)))
                {
                        error("Absolute error " + to_string(sum_error));
                }
        }
};
}
