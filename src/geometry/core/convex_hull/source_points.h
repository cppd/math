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

#include "integer_types.h"

#include <src/com/error.h>
#include <src/com/print.h>
#include <src/com/random/pcg.h>
#include <src/com/shuffle.h>
#include <src/com/type/find.h>
#include <src/numerical/vector.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <type_traits>
#include <unordered_set>
#include <vector>

namespace ns::geometry::core::convex_hull
{
namespace source_points_implementation
{
template <std::size_t N, typename T>
class Transform final
{
        static_assert(std::is_integral_v<T>);

        const std::vector<numerical::Vector<N, float>>* points_;
        T max_value_;
        numerical::Vector<N, float> min_;
        double scale_;

public:
        Transform(const std::vector<numerical::Vector<N, float>>* const points, const std::type_identity_t<T> max_value)
                : points_(points),
                  max_value_(max_value)
        {
                ASSERT(points);
                ASSERT(!points->empty());
                ASSERT(0 < max_value);

                numerical::Vector<N, float> min = (*points)[0];
                numerical::Vector<N, float> max = (*points)[0];
                for (std::size_t i = 1; i < points->size(); ++i)
                {
                        for (std::size_t n = 0; n < N; ++n)
                        {
                                min[n] = std::min(min[n], (*points)[i][n]);
                                max[n] = std::max(max[n], (*points)[i][n]);
                        }
                }

                const double max_d = (max - min).norm_infinity();
                if (!(max_d != 0))
                {
                        error("No distinct points found");
                }

                min_ = min;
                scale_ = max_value / max_d;
        }

        [[nodiscard]] numerical::Vector<N, T> to_integer(const std::size_t index) const
        {
                ASSERT(index < points_->size());

                const numerical::Vector<N, float>& point = (*points_)[index];

                numerical::Vector<N, T> res;
                for (std::size_t i = 0; i < N; ++i)
                {
                        const long long value = std::llround((point[i] - min_[i]) * scale_);
                        if (!(value >= 0 && value <= max_value_))
                        {
                                error("Error converting to integer: " + to_string(value) + " is not in the range [0, "
                                      + to_string(max_value_) + "]");
                        }
                        res[i] = value;
                }
                return res;
        }
};

template <std::size_t N, std::size_t BITS>
class Points final
{
        using T = LeastSignedInteger<BITS>;
        static constexpr T MAX{(1ull << BITS) - 1};

        std::vector<numerical::Vector<N, T>> points_;
        std::vector<int> map_;

public:
        explicit Points(const std::vector<numerical::Vector<N, float>>& points)
        {
                points_.reserve(points.size());
                map_.reserve(points.size());

                const Transform<N, T> transform{&points, MAX};

                std::unordered_set<numerical::Vector<N, T>> set(points.size());

                for (std::size_t i = 0; i < points.size(); ++i)
                {
                        const numerical::Vector<N, T> integer_value = transform.to_integer(i);
                        if (set.insert(integer_value).second)
                        {
                                points_.push_back(integer_value);
                                map_.push_back(i);
                        }
                }

                shuffle(PCG(points_.size()), &points_, &map_);
        }

        [[nodiscard]] const std::vector<numerical::Vector<N, T>>& points() const
        {
                return points_;
        }

        template <std::size_t M>
        [[nodiscard]] std::array<int, M> restore_indices(const std::array<int, M>& indices) const
        {
                std::array<int, M> res;
                for (std::size_t i = 0; i < M; ++i)
                {
                        res[i] = map_[indices[i]];
                }
                return res;
        }

        [[nodiscard]] int restore_index(const int index) const
        {
                return map_[index];
        }
};
}

template <std::size_t N>
using ConvexHullPoints = source_points_implementation::Points<N, CONVEX_HULL_BITS>;

template <std::size_t N>
using DelaunayPoints = source_points_implementation::Points<N, DELAUNAY_BITS>;
}
