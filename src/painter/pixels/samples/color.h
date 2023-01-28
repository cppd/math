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

#include <algorithm>
#include <array>

namespace ns::painter::pixels::samples
{
template <typename Color>
class ColorSamples final
{
        static constexpr std::size_t COUNT = 2;
        static_assert(COUNT >= 2);
        static_assert(COUNT % 2 == 0);

        static constexpr typename Color::DataType EMPTY = -static_cast<int>(COUNT) - 1;

        Color color_sum_;
        std::array<Color, COUNT> colors_;

        typename Color::DataType weight_sum_;
        std::array<typename Color::DataType, COUNT> weights_;

        std::array<typename Color::DataType, COUNT> contributions_;

public:
        static constexpr std::size_t size()
        {
                return COUNT;
        }

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
                const typename Color::DataType weight_sum,
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
                return full() ? COUNT : (empty() ? 0 : -weight_sum_);
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

        [[nodiscard]] typename Color::DataType weight_sum() const
        {
                ASSERT(full());
                return weight_sum_;
        }

        [[nodiscard]] typename Color::DataType weight(const std::size_t index) const
        {
                ASSERT(index < count());
                return weights_[index];
        }

        [[nodiscard]] typename Color::DataType contribution(const std::size_t index) const
        {
                ASSERT(index < count());
                return contributions_[index];
        }

        [[nodiscard]] const std::array<Color, COUNT>& colors() const
        {
                ASSERT(!empty());
                return colors_;
        }

        [[nodiscard]] const std::array<typename Color::DataType, COUNT>& weights() const
        {
                ASSERT(!empty());
                return weights_;
        }

        [[nodiscard]] const std::array<typename Color::DataType, COUNT>& contributions() const
        {
                ASSERT(!empty());
                return contributions_;
        }
};
}
