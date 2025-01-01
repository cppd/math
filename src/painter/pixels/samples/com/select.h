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

#include <cstddef>
#include <optional>
#include <vector>

namespace ns::painter::pixels::samples::com
{
template <bool COLOR, typename Color, typename T, typename Select>
void select_samples(const std::vector<std::optional<Color>>& colors, const std::vector<T>& weights, const Select select)
{
        ASSERT(colors.size() == weights.size());

        for (std::size_t i = 0; i < colors.size(); ++i)
        {
                if (COLOR != colors[i].has_value())
                {
                        continue;
                }

                if (!(weights[i] > 0))
                {
                        continue;
                }

                select(i);
        }
}
}
