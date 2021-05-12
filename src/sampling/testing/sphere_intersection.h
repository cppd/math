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

#include "sphere_facet.h"

#include <src/com/error.h>
#include <src/com/random/engine.h>
#include <src/geometry/spatial/object_tree.h>

#include <sstream>
#include <vector>

namespace ns::sampling::testing
{
template <std::size_t N, typename T, typename RandomEngine>
class SphereIntersection final
{
        const geometry::ObjectTree<SphereFacet<N, T>>* const m_tree;
        const std::vector<SphereFacet<N, T>>* const m_facets;

        RandomEngine m_random_engine = create_engine<RandomEngine>();
        long long m_intersection_count = 0;
        long long m_missed_intersection_count = 0;

public:
        SphereIntersection(
                const geometry::ObjectTree<SphereFacet<N, T>>* tree,
                const std::vector<SphereFacet<N, T>>* facets)
                : m_tree(tree), m_facets(facets)
        {
        }

        template <typename RandomVector>
        std::tuple<std::size_t, Vector<N, T>> find(const RandomVector& random_vector)
        {
                while (true)
                {
                        const Ray<N, T> ray(Vector<N, T>(0), random_vector(m_random_engine));

                        const std::optional<T> root_distance = m_tree->intersect_root(ray);
                        ASSERT(root_distance && *root_distance == 0);

                        const std::optional<std::tuple<T, const SphereFacet<N, T>*>> v =
                                m_tree->intersect(ray, *root_distance);
                        if (v)
                        {
                                ++m_intersection_count;
                                const std::size_t index = std::get<1>(*v) - m_facets->data();
                                ASSERT(index < m_facets->size());
                                return {index, ray.dir()};
                        }
                        ++m_missed_intersection_count;
                }
        }

        long long intersection_count() const
        {
                return m_intersection_count;
        }

        long long missed_intersection_count() const
        {
                return m_missed_intersection_count;
        }
};

inline void check_sphere_intersections(long long intersection_count, long long missed_intersection_count)
{
        const long long sample_count = intersection_count + missed_intersection_count;
        if (sample_count < 1'000'000)
        {
                error("Too few samples " + to_string(sample_count));
        }

        const long long max_missed_count = std::ceil(sample_count * 1e-6);
        if (missed_intersection_count >= max_missed_count)
        {
                std::ostringstream oss;
                oss << "Too many missed intersections" << '\n';
                oss << "missed intersections = " << missed_intersection_count << '\n';
                oss << "all samples = " << sample_count << '\n';
                oss << "missed/all = " << (double(missed_intersection_count) / sample_count);
                error(oss.str());
        }
}
}
