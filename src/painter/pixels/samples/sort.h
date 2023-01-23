/*
Copyright (C) 2017-2023 Topological Manifold

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

#include <tuple>
#include <vector>

namespace ns::painter::pixels::samples
{
template <typename Less, typename Greater>
[[nodiscard]] std::tuple<std::size_t, std::size_t> find_min_max(
        const std::vector<int>& values,
        const Less less,
        const Greater greater)
{
        ASSERT(values.size() >= 2);

        int min_i = 0;
        int max_i = 0;

        for (int i = 1, size = static_cast<int>(values.size()); i < size; ++i)
        {
                if (less(i, min_i))
                {
                        min_i = i;
                }

                if (greater(i, max_i))
                {
                        max_i = i;
                }
        }

        if (min_i == max_i)
        {
                // all elements are equal
                return {0, 1};
        }
        return {min_i, max_i};
}

}
