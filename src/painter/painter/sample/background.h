/*
Copyright (C) 2017-2022 Topological Manifold

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

namespace ns::painter::sample
{
template <typename Color>
class BackgroundSamples final
{
        typename Color::DataType sum_weight_{0};
        typename Color::DataType min_weight_{Limits<decltype(min_weight_)>::max()};
        typename Color::DataType max_weight_{Limits<decltype(max_weight_)>::lowest()};

public:
        BackgroundSamples()
        {
        }

        template <typename T>
                requires(std::is_same_v<T, typename Color::DataType>)
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

        void merge(const BackgroundSamples<Color>& samples)
        {
                ASSERT(!samples.empty());

                sum_weight_ += samples.sum_weight_;

                if (samples.min_weight_ < min_weight_)
                {
                        sum_weight_ += min_weight_;
                        min_weight_ = samples.min_weight_;
                }
                else
                {
                        sum_weight_ += samples.min_weight_;
                }

                if (samples.max_weight_ > max_weight_)
                {
                        sum_weight_ += max_weight_;
                        max_weight_ = samples.max_weight_;
                }
                else
                {
                        sum_weight_ += samples.max_weight_;
                }
        }
};

template <typename Color, typename Weight>
[[nodiscard]] std::optional<BackgroundSamples<Color>> make_background_samples(
        const std::vector<std::optional<Color>>& colors,
        const std::vector<Weight>& color_weights)
{
        using T = typename Color::DataType;

        ASSERT(colors.size() == color_weights.size());

        thread_local std::vector<T> weights;

        weights.clear();

        T min = Limits<T>::max();
        T max = Limits<T>::lowest();
        std::size_t min_i = Limits<std::size_t>::max();
        std::size_t max_i = Limits<std::size_t>::max();

        for (std::size_t i = 0; i < colors.size(); ++i)
        {
                if (colors[i])
                {
                        continue;
                }

                const T weight = color_weights[i];

                if (!(weight > 0))
                {
                        continue;
                }

                weights.push_back(weight);

                if (weight < min)
                {
                        min = weight;
                        min_i = weights.size() - 1;
                }

                if (weight > max)
                {
                        max = weight;
                        max_i = weights.size() - 1;
                }
        }

        if (weights.empty())
        {
                return std::nullopt;
        }

        ASSERT(min_i < weights.size());
        ASSERT(max_i < weights.size());

        T sum_weight = 0;

        if (weights.size() > 2)
        {
                for (std::size_t i = 0; i < weights.size(); ++i)
                {
                        if (i != min_i && i != max_i)
                        {
                                sum_weight += weights[i];
                        }
                }
        }

        return BackgroundSamples<Color>{sum_weight, weights[min_i], weights[max_i]};
}
}
