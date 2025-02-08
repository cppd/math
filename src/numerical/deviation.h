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

#include <src/com/error.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace ns::numerical
{
template <typename T>
struct MedianAbsoluteDeviation final
{
        T median;
        T deviation;
};

template <typename T>
[[nodiscard]] MedianAbsoluteDeviation<T> median_absolute_deviation(std::vector<T> data)
{
        static_assert(std::is_floating_point_v<T>);

        if (data.empty())
        {
                error("No data for median absolute deviation");
        }

        const std::size_t m = data.size() / 2;

        std::ranges::nth_element(data, data.begin() + m);

        const T median = [&]
        {
                if ((data.size() % 2) == 1)
                {
                        return data[m];
                }
                const auto iter = std::max_element(data.begin(), data.begin() + m);
                return (*iter + data[m]) / 2;
        }();

        for (T& v : data)
        {
                v = std::abs(v - median);
        }

        std::ranges::nth_element(data, data.begin() + m);
        const T deviation = data[m];

        return {
                .median = median,
                .deviation = deviation,
        };
}

template <typename T>
[[nodiscard]] T standard_deviation(const MedianAbsoluteDeviation<T>& mad)
{
        // mad = sigma * sqrt(2) * inverse_erf(1/2)
        // k = 1 / (sqrt(2) * inverse_erf(1/2))
        static constexpr T K = 1.4826022185056018605L;

        return K * mad.deviation;
}
}
