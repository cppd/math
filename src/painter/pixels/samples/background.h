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
#include <src/com/type/limit.h>

#include <optional>
#include <vector>

namespace ns::painter::pixels::samples
{
template <typename Color>
class BackgroundSamples final
{
        static constexpr typename Color::DataType EMPTY = -1;
        static constexpr typename Color::DataType SUM_ONLY = -2;

        typename Color::DataType sum_weight_{0};
        typename Color::DataType min_weight_{EMPTY};
        typename Color::DataType max_weight_{EMPTY};

public:
        BackgroundSamples()
        {
        }

        template <typename T>
                requires (std::is_same_v<T, typename Color::DataType>)
        explicit BackgroundSamples(const T sum_weight)
                : sum_weight_(sum_weight),
                  min_weight_(SUM_ONLY),
                  max_weight_(SUM_ONLY)
        {
        }

        template <typename T>
                requires (std::is_same_v<T, typename Color::DataType>)
        BackgroundSamples(const T min_weight, const T max_weight)
                : BackgroundSamples(T{0}, min_weight, max_weight)
        {
                ASSERT(max_weight_ >= min_weight_);
                ASSERT(min_weight_ > 0);
        }

        template <typename T>
                requires (std::is_same_v<T, typename Color::DataType>)
        BackgroundSamples(const T sum_weight, const T min_weight, const T max_weight)
                : sum_weight_(sum_weight),
                  min_weight_(min_weight),
                  max_weight_(max_weight)
        {
                ASSERT(max_weight_ >= min_weight_);
                ASSERT(min_weight_ > 0);
        }

        [[nodiscard]] bool empty() const
        {
                return min_weight_ == EMPTY;
        }

        [[nodiscard]] bool sum_only() const
        {
                return min_weight_ == SUM_ONLY;
        }

        [[nodiscard]] bool full() const
        {
                return min_weight_ > 0;
        }

        [[nodiscard]] typename Color::DataType sum_weight() const
        {
                ASSERT(!empty());
                return sum_weight_;
        }

        [[nodiscard]] typename Color::DataType min_weight() const
        {
                ASSERT(full());
                return min_weight_;
        }

        [[nodiscard]] typename Color::DataType max_weight() const
        {
                ASSERT(full());
                return max_weight_;
        }
};

template <typename T, typename Color>
[[nodiscard]] std::optional<BackgroundSamples<Color>> make_background_samples(
        const std::vector<std::optional<Color>>& colors,
        const std::vector<T>& color_weights);

template <typename Color>
[[nodiscard]] BackgroundSamples<Color> merge_background_samples(
        const BackgroundSamples<Color>& a,
        const BackgroundSamples<Color>& b);
}
