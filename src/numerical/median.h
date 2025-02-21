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
#include <cstddef>
#include <type_traits>
#include <vector>

namespace ns::numerical
{
template <typename T>
[[nodiscard]] T median(std::vector<T>* const data)
{
        static_assert(std::is_floating_point_v<T>);

        if (!data || data->empty())
        {
                error("No data for median");
        }

        const std::size_t m = data->size() / 2;

        std::ranges::nth_element(*data, data->begin() + m);

        if (data->size() & 1u)
        {
                return (*data)[m];
        }

        const auto iter = std::max_element(data->begin(), data->begin() + m);
        return (*iter + (*data)[m]) / 2;
}
}
