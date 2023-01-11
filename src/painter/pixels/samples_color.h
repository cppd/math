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

#include <src/com/type/limit.h>

#include <optional>
#include <vector>

namespace ns::painter::pixels
{
template <typename Color>
struct ColorSamples final
{
        Color sum{0};
        Color min{0};
        Color max{0};

        typename Color::DataType sum_weight{0};
        typename Color::DataType min_weight{0};
        typename Color::DataType max_weight{0};

        typename Color::DataType min_contribution{Limits<decltype(min_contribution)>::infinity()};
        typename Color::DataType max_contribution{-Limits<decltype(max_contribution)>::infinity()};

        [[nodiscard]] bool empty() const
        {
                return min_contribution > max_contribution;
        }
};

template <typename T, typename Color>
[[nodiscard]] std::optional<ColorSamples<Color>> make_color_samples(
        const std::vector<std::optional<Color>>& colors,
        const std::vector<T>& color_weights);
}
