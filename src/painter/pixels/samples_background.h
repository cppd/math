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
class BackgroundSamples final
{
        typename Color::DataType sum_weight_{0};
        typename Color::DataType min_weight_{Limits<typename Color::DataType>::infinity()};
        typename Color::DataType max_weight_{-Limits<typename Color::DataType>::infinity()};

public:
        BackgroundSamples()
        {
        }

        template <typename T>
                requires (std::is_same_v<T, typename Color::DataType>)
        BackgroundSamples(const T sum_weight, const T min_weight, const T max_weight)
                : sum_weight_(sum_weight),
                  min_weight_(min_weight),
                  max_weight_(max_weight)
        {
        }

        [[nodiscard]] bool empty() const
        {
                return min_weight_ > max_weight_;
        }

        [[nodiscard]] typename Color::DataType sum_weight() const
        {
                return sum_weight_;
        }

        [[nodiscard]] typename Color::DataType min_weight() const
        {
                return min_weight_;
        }

        [[nodiscard]] typename Color::DataType max_weight() const
        {
                return max_weight_;
        }
};

template <typename T, typename Color>
[[nodiscard]] std::optional<BackgroundSamples<Color>> make_background_samples(
        const std::vector<std::optional<Color>>& colors,
        const std::vector<T>& color_weights);
}
