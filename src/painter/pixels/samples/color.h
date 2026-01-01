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

#include <src/com/error.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <type_traits>

namespace ns::painter::pixels::samples
{
template <std::size_t COUNT, typename Color>
class ColorSamples final
{
        static_assert(COUNT >= 2);
        static_assert(COUNT % 2 == 0);
        static_assert(std::is_signed_v<typename Color::DataType>);

        static constexpr Color::DataType EMPTY = -static_cast<int>(COUNT) - 1;

        Color color_sum_;
        std::array<Color, COUNT> colors_;

        Color::DataType weight_sum_;
        std::array<typename Color::DataType, COUNT> weights_;

        std::array<typename Color::DataType, COUNT> contributions_;

public:
        ColorSamples()
                : weight_sum_(EMPTY)
        {
        }

        ColorSamples(
                const std::array<Color, COUNT>& colors,
                const std::array<typename Color::DataType, COUNT>& weights,
                const std::array<typename Color::DataType, COUNT>& contributions,
                const std::size_t count)
                : colors_(colors),
                  weight_sum_(-static_cast<int>(count)),
                  weights_(weights),
                  contributions_(contributions)
        {
                ASSERT(weight_sum_ < 0);
                ASSERT(weight_sum_ > EMPTY);
                ASSERT(std::is_sorted(contributions_.cbegin(), contributions_.cbegin() + count));
        }

        ColorSamples(
                const Color& color_sum,
                const std::array<Color, COUNT>& colors,
                const Color::DataType weight_sum,
                const std::array<typename Color::DataType, COUNT>& weights,
                const std::array<typename Color::DataType, COUNT>& contributions)
                : color_sum_(color_sum),
                  colors_(colors),
                  weight_sum_(weight_sum),
                  weights_(weights),
                  contributions_(contributions)
        {
                ASSERT(weight_sum_ >= 0);
                ASSERT(std::is_sorted(contributions_.cbegin(), contributions_.cend()));
        }

        [[nodiscard]] bool empty() const
        {
                return weight_sum_ == EMPTY;
        }

        [[nodiscard]] bool full() const
        {
                return weight_sum_ >= 0;
        }

        [[nodiscard]] std::size_t count() const
        {
                if (full())
                {
                        return COUNT;
                }
                return empty() ? 0 : -weight_sum_;
        }

        [[nodiscard]] const Color& color_sum() const
        {
                ASSERT(full());
                return color_sum_;
        }

        [[nodiscard]] const Color& color(const std::size_t index) const
        {
                ASSERT(index < count());
                return colors_[index];
        }

        [[nodiscard]] Color::DataType weight_sum() const
        {
                ASSERT(full());
                return weight_sum_;
        }

        [[nodiscard]] Color::DataType weight(const std::size_t index) const
        {
                ASSERT(index < count());
                return weights_[index];
        }

        [[nodiscard]] Color::DataType contribution(const std::size_t index) const
        {
                ASSERT(index < count());
                return contributions_[index];
        }
};
}
