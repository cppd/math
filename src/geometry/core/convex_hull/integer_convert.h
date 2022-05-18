/*
Copyright (C) 2017-2022 Topological Manifold

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
#include <src/com/print.h>
#include <src/numerical/vector.h>

#include <algorithm>
#include <unordered_set>
#include <vector>

namespace ns::geometry::convex_hull
{
namespace integer_convert_implementation
{
template <std::size_t N, typename IntegerType>
class Transform
{
        const std::vector<Vector<N, float>>* points_;
        IntegerType max_value_;
        Vector<N, float> min_;
        double scale_;

public:
        Transform(const std::vector<Vector<N, float>>* const points, const std::type_identity_t<IntegerType>& max_value)
        {
                ASSERT(points);
                ASSERT(!points->empty());
                ASSERT(0 < max_value);

                Vector<N, float> min = (*points)[0];
                Vector<N, float> max = (*points)[0];
                for (std::size_t i = 1; i < points->size(); ++i)
                {
                        for (std::size_t n = 0; n < N; ++n)
                        {
                                min[n] = std::min(min[n], (*points)[i][n]);
                                max[n] = std::max(max[n], (*points)[i][n]);
                        }
                }

                const double max_d = (max - min).norm_infinity();
                if (max_d == 0)
                {
                        error("All points are equal to each other");
                }

                points_ = points;
                max_value_ = max_value;
                min_ = min;
                scale_ = max_value / max_d;
        }

        [[nodiscard]] Vector<N, IntegerType> to_integer(const std::size_t i) const
        {
                ASSERT(i < points_->size());

                const Vector<N, double> float_value = to_vector<double>((*points_)[i] - min_) * scale_;

                Vector<N, IntegerType> integer_value;
                for (std::size_t n = 0; n < N; ++n)
                {
                        const long long ll = std::llround(float_value[n]);
                        if (!(ll >= 0 && ll <= max_value_))
                        {
                                error("Error converting to integer: " + to_string(ll) + " is not in the range [0, "
                                      + to_string(max_value_) + "]");
                        }
                        integer_value[n] = ll;
                }
                return integer_value;
        }
};
}

template <std::size_t N, typename IntegerType>
void convert_to_unique_integer(
        const std::vector<Vector<N, float>>& source_points,
        const IntegerType& max_value,
        std::vector<Vector<N, IntegerType>>* const points,
        std::vector<int>* const map)
{
        ASSERT(points && map);

        points->clear();
        points->reserve(source_points.size());

        map->clear();
        map->reserve(source_points.size());

        const integer_convert_implementation::Transform<N, IntegerType> transform{&source_points, max_value};

        std::unordered_set<Vector<N, IntegerType>> set(source_points.size());

        for (std::size_t i = 0; i < source_points.size(); ++i)
        {
                const Vector<N, IntegerType> integer_value = transform.to_integer(i);
                if (set.insert(integer_value).second)
                {
                        points->push_back(integer_value);
                        map->push_back(i);
                }
        }
}
}
