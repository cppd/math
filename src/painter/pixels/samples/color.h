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
class ColorSamples final
{
        static constexpr typename Color::DataType EMPTY = -1;
        static constexpr typename Color::DataType SUM_ONLY = -2;

        Color sum_{0};
        Color min_{0};
        Color max_{0};

        typename Color::DataType sum_weight_{0};
        typename Color::DataType min_weight_{EMPTY};
        typename Color::DataType max_weight_{EMPTY};

        typename Color::DataType min_contribution_{EMPTY};
        typename Color::DataType max_contribution_{EMPTY};

public:
        ColorSamples()
        {
        }

        template <typename T>
                requires (std::is_same_v<T, typename Color::DataType>)
        ColorSamples(const Color& sum, const T sum_weight, const T sum_contribution)
                : sum_(sum),
                  min_(0),
                  max_(0),
                  sum_weight_(sum_weight),
                  min_weight_(SUM_ONLY),
                  max_weight_(SUM_ONLY),
                  min_contribution_(sum_contribution),
                  max_contribution_(sum_contribution)
        {
                ASSERT(sum_contribution >= 0);
        }

        template <typename T>
                requires (std::is_same_v<T, typename Color::DataType>)
        ColorSamples(
                const Color& min,
                const Color& max,
                const T min_weight,
                const T max_weight,
                const T min_contribution,
                const T max_contribution)
                : sum_(0),
                  min_(min),
                  max_(max),
                  sum_weight_(0),
                  min_weight_(min_weight),
                  max_weight_(max_weight),
                  min_contribution_(min_contribution),
                  max_contribution_(max_contribution)
        {
                ASSERT(min_weight_ > 0);
                ASSERT(max_weight_ > 0);
                ASSERT(max_contribution_ >= min_contribution_);
                ASSERT(min_contribution_ >= 0);
        }

        template <typename T>
                requires (std::is_same_v<T, typename Color::DataType>)
        ColorSamples(
                const Color& sum,
                const Color& min,
                const Color& max,
                const T sum_weight,
                const T min_weight,
                const T max_weight,
                const T min_contribution,
                const T max_contribution)
                : sum_(sum),
                  min_(min),
                  max_(max),
                  sum_weight_(sum_weight),
                  min_weight_(min_weight),
                  max_weight_(max_weight),
                  min_contribution_(min_contribution),
                  max_contribution_(max_contribution)
        {
                ASSERT(min_weight_ > 0);
                ASSERT(max_weight_ > 0);
                ASSERT(max_contribution_ >= min_contribution_);
                ASSERT(min_contribution_ >= 0);
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

        [[nodiscard]] const Color& sum() const
        {
                ASSERT(!empty());
                return sum_;
        }

        [[nodiscard]] const Color& min() const
        {
                ASSERT(full());
                return min_;
        }

        [[nodiscard]] const Color& max() const
        {
                ASSERT(full());
                return max_;
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

        [[nodiscard]] typename Color::DataType sum_contribution() const
        {
                ASSERT(sum_only());
                ASSERT(min_contribution_ == max_contribution_);
                return min_contribution_;
        }

        [[nodiscard]] typename Color::DataType min_contribution() const
        {
                ASSERT(full());
                return min_contribution_;
        }

        [[nodiscard]] typename Color::DataType max_contribution() const
        {
                ASSERT(full());
                return max_contribution_;
        }
};

template <typename T, typename Color>
[[nodiscard]] std::optional<ColorSamples<Color>> make_color_samples(
        const std::vector<std::optional<Color>>& colors,
        const std::vector<T>& color_weights);

template <typename Color>
[[nodiscard]] ColorSamples<Color> merge_color_samples(const ColorSamples<Color>& a, const ColorSamples<Color>& b);
}
