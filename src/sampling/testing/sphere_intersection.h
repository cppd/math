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

#include "sphere_mesh.h"

#include <src/com/error.h>
#include <src/com/print.h>
#include <src/numerical/ray.h>
#include <src/numerical/vector.h>

#include <cmath>
#include <cstddef>
#include <sstream>
#include <tuple>

namespace ns::sampling::testing
{
template <std::size_t N, typename T>
class SphereIntersection final
{
        const SphereMesh<N, T>* const sphere_mesh_;

        long long intersection_count_ = 0;
        long long missed_intersection_count_ = 0;

public:
        explicit SphereIntersection(const SphereMesh<N, T>* const sphere_mesh)
                : sphere_mesh_(sphere_mesh)
        {
        }

        template <typename RandomVector>
        std::tuple<unsigned, numerical::Vector<N, T>> find(const RandomVector& random_vector)
        {
                for (int i = 0; i < 10; ++i)
                {
                        const numerical::Ray<N, T> ray(numerical::Vector<N, T>(0), random_vector());
                        const auto index = sphere_mesh_->intersect(ray);
                        if (index)
                        {
                                ++intersection_count_;
                                return {*index, ray.dir()};
                        }
                        ++missed_intersection_count_;
                }
                error("Too many missed intersections");
        }

        [[nodiscard]] long long intersection_count() const
        {
                return intersection_count_;
        }

        [[nodiscard]] long long missed_intersection_count() const
        {
                return missed_intersection_count_;
        }
};

inline void check_sphere_intersections(const long long intersection_count, const long long missed_intersection_count)
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
                oss << "missed/all = " << (static_cast<double>(missed_intersection_count) / sample_count);
                error(oss.str());
        }
}
}
