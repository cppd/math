/*
Copyright (C) 2017-2026 Topological Manifold

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

#include <src/com/error.h>
#include <src/numerical/vector.h>

#include <cstddef>
#include <cstdlib>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

namespace ns::storage::repository
{
template <std::size_t N>
class QuantizedPoints final
{
        const int point_quantization_;
        std::vector<numerical::Vector<N, float>> points_;
        std::unordered_set<numerical::Vector<N, int>> integer_points_;

        template <typename T>
        numerical::Vector<N, int> to_integer(const numerical::Vector<N, T>& v) const
        {
                static_assert(std::is_floating_point_v<T>);

                numerical::Vector<N, int> res;
                for (std::size_t i = 0; i < N; ++i)
                {
                        res[i] = std::lround(v[i] * point_quantization_);
                }
                return res;
        }

public:
        QuantizedPoints(const int point_quantization, const unsigned point_count)
                : point_quantization_(point_quantization)
        {
                points_.reserve(point_count);
                integer_points_.reserve(point_count);
        }

        template <typename T>
        void add(const numerical::Vector<N, T>& p)
        {
                const numerical::Vector<N, int> integer_point = to_integer(p);
                if (!integer_points_.contains(integer_point))
                {
                        integer_points_.insert(integer_point);
                        points_.push_back(to_vector<float>(p));
                }
        }

        [[nodiscard]] unsigned size() const
        {
                return points_.size();
        }

        [[nodiscard]] std::vector<numerical::Vector<N, float>> release()
        {
                ASSERT(integer_points_.size() == points_.size());
                ASSERT(points_.size() == std::unordered_set(points_.cbegin(), points_.cend()).size());

                integer_points_.clear();

                return std::move(points_);
        }
};
}
