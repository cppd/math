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

/*
Time-effcient algorithms for two highly robust estimators of scale.
Christophe Croux, Peter J. Rousseeuw.

Alternatives to the Median Absolute Deviation.
Peter J. Rousseeuw, Christophe Croux.
*/

#pragma once

#include "median.h"
#include "median_sorted.h"

#include <src/com/error.h>

#include <algorithm>
#include <cstddef>
#include <vector>

namespace ns::statistics
{
template <typename T>
[[nodiscard]] T estimator_sn(std::vector<T> data)
{
        static_assert(std::is_floating_point_v<T>);

        if (data.size() <= 1)
        {
                error("No data for estimator Sn");
        }

        std::ranges::sort(data);

        std::vector<T> v;
        v.reserve(data.size());

        for (std::size_t i = 0; i < data.size(); ++i)
        {
                const std::size_t s1 = i;
                const std::size_t s2 = data.size() - 1 - i;

                const std::size_t prev = i - 1;
                const std::size_t next = i + 1;

                const auto f1 = [&](const std::size_t index)
                {
                        ASSERT(s1 > 0 && index < s1);
                        return data[i] - data[prev - index];
                };

                const auto f2 = [&](const std::size_t index)
                {
                        ASSERT(s2 > 0 && index < s2);
                        return data[next + index] - data[i];
                };

                const T m = median_of_sorted_data(f1, s1, f2, s2);
                v.push_back(m);
        }

        return median(&v);
}

template <typename T>
[[nodiscard]] T estimator_sn_standard_deviation(const T sn)
{
        static constexpr T CORRECTION = 1.1926L;

        return CORRECTION * sn;
}
}
