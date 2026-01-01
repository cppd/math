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

#include <algorithm>
#include <cstddef>
#include <vector>

namespace ns::painter::pixels::samples::com
{
template <std::size_t COUNT, typename T, typename Less, typename Greater>
void partial_sort(std::vector<T>* const data, const Less less, const Greater greater)
{
        static_assert(COUNT >= 2);
        static_assert(COUNT % 2 == 0);

        if (data->size() <= 1)
        {
                return;
        }

        if (data->size() <= COUNT * 2)
        {
                std::sort(data->begin(), data->end(), less);
                return;
        }

        std::partial_sort(data->begin(), data->begin() + COUNT / 2, data->end(), less);

        std::partial_sort(data->rbegin(), data->rbegin() + COUNT / 2, data->rend() - COUNT / 2, greater);
}
}
