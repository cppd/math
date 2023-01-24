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

#include <algorithm>
#include <vector>

namespace ns::painter::pixels::samples
{
template <std::size_t COUNT, typename T, typename Less, typename Greater>
void sort_samples(std::vector<T>* const samples, const Less less, const Greater greater)
{
        static_assert(COUNT >= 2);
        static_assert(COUNT % 2 == 0);

        if (samples->size() <= 1)
        {
                return;
        }

        if (samples->size() <= COUNT * 2)
        {
                std::sort(samples->begin(), samples->end(), less);
                return;
        }

        std::partial_sort(samples->begin(), samples->begin() + COUNT / 2, samples->end(), less);
        std::partial_sort(samples->rbegin(), samples->rbegin() + COUNT / 2, samples->rend() - COUNT / 2, greater);
}
}
