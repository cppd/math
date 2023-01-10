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

namespace ns::painter::pixels
{
template <typename Color>
class ColorSamples final
{
        Color sum_{0};
        Color min_{0};
        Color max_{0};

        typename Color::DataType sum_weight_{0};
        typename Color::DataType min_contribution_{Limits<decltype(min_contribution_)>::max()};
        typename Color::DataType min_weight_{0};
        typename Color::DataType max_contribution_{Limits<decltype(max_contribution_)>::lowest()};
        typename Color::DataType max_weight_{0};

public:
        ColorSamples()
        {
        }

        template <typename T>
                requires (std::is_same_v<T, typename Color::DataType>)
        ColorSamples(
                const Color& sum,
                const Color& min,
                const Color& max,
                const T sum_weight,
                const T min_contribution,
                const T min_weight,
                const T max_contribution,
                const T max_weight)
                : sum_(sum),
                  min_(min),
                  max_(max),
                  sum_weight_(sum_weight),
                  min_contribution_(min_contribution),
                  min_weight_(min_weight),
                  max_contribution_(max_contribution),
                  max_weight_(max_weight)
        {
        }

        [[nodiscard]] bool empty() const
        {
                return min_contribution_ > max_contribution_;
        }

        [[nodiscard]] const Color& sum() const
        {
                return sum_;
        }

        [[nodiscard]] const Color& min() const
        {
                return min_;
        }

        [[nodiscard]] const Color& max() const
        {
                return max_;
        }

        [[nodiscard]] typename Color::DataType sum_weight() const
        {
                return sum_weight_;
        }

        [[nodiscard]] typename Color::DataType min_contribution() const
        {
                return min_contribution_;
        }

        [[nodiscard]] typename Color::DataType min_weight() const
        {
                return min_weight_;
        }

        [[nodiscard]] typename Color::DataType max_contribution() const
        {
                return max_contribution_;
        }

        [[nodiscard]] typename Color::DataType max_weight() const
        {
                return max_weight_;
        }

        void merge(const ColorSamples<Color>& samples)
        {
                ASSERT(!samples.empty());

                sum_ += samples.sum_;
                sum_weight_ += samples.sum_weight_;

                if (samples.min_contribution_ < min_contribution_)
                {
                        sum_ += min_;
                        sum_weight_ += min_weight_;
                        min_ = samples.min_;
                        min_contribution_ = samples.min_contribution_;
                        min_weight_ = samples.min_weight_;
                }
                else
                {
                        sum_ += samples.min_;
                        sum_weight_ += samples.min_weight_;
                }

                if (samples.max_contribution_ > max_contribution_)
                {
                        sum_ += max_;
                        sum_weight_ += max_weight_;
                        max_ = samples.max_;
                        max_contribution_ = samples.max_contribution_;
                        max_weight_ = samples.max_weight_;
                }
                else
                {
                        sum_ += samples.max_;
                        sum_weight_ += samples.max_weight_;
                }
        }
};

template <typename T, typename Color>
[[nodiscard]] std::optional<ColorSamples<Color>> make_color_samples(
        const std::vector<std::optional<Color>>& colors,
        const std::vector<T>& color_weights);
}
