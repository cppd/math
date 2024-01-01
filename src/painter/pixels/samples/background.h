/*
Copyright (C) 2017-2024 Topological Manifold

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
class BackgroundSamples final
{
        static_assert(COUNT >= 2);
        static_assert(COUNT % 2 == 0);
        static_assert(std::is_signed_v<typename Color::DataType>);

        static constexpr Color::DataType EMPTY = -static_cast<int>(COUNT) - 1;

        Color::DataType weight_sum_;
        std::array<typename Color::DataType, COUNT> weights_;

public:
        BackgroundSamples()
                : weight_sum_(EMPTY)
        {
        }

        BackgroundSamples(const std::array<typename Color::DataType, COUNT>& weights, const std::size_t count)
                : weight_sum_(-static_cast<int>(count)),
                  weights_(weights)
        {
                ASSERT(weight_sum_ < 0);
                ASSERT(weight_sum_ > EMPTY);
                ASSERT(std::is_sorted(weights_.cbegin(), weights_.cbegin() + count));
        }

        BackgroundSamples(const Color::DataType weight_sum, const std::array<typename Color::DataType, COUNT>& weights)
                : weight_sum_(weight_sum),
                  weights_(weights)
        {
                ASSERT(weight_sum_ >= 0);
                ASSERT(std::is_sorted(weights_.cbegin(), weights_.cend()));
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
};
}
