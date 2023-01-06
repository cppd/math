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

#include "color_contribution.h"

#include <src/com/error.h>
#include <src/com/type/limit.h>

#include <optional>
#include <tuple>
#include <vector>

namespace ns::painter::pixels
{
namespace samples_color_implementation
{
template <typename Color, typename Weight>
[[nodiscard]] std::tuple<std::size_t, std::size_t> select_colors_and_find_min_max(
        const std::vector<std::optional<Color>>& colors,
        const std::vector<Weight>& color_weights,
        std::vector<Color>* const samples,
        std::vector<typename Color::DataType>* const contributions,
        std::vector<typename Color::DataType>* const weights)
{
        using T = typename Color::DataType;
        static_assert(std::is_floating_point_v<T>);

        ASSERT(colors.size() == color_weights.size());

        samples->clear();
        contributions->clear();
        weights->clear();

        T min = Limits<T>::infinity();
        T max = -Limits<T>::infinity();
        std::size_t min_i = -1;
        std::size_t max_i = -1;

        for (std::size_t i = 0; i < colors.size(); ++i)
        {
                if (!colors[i])
                {
                        continue;
                }

                const T weight = color_weights[i];

                if (!(weight > 0))
                {
                        continue;
                }

                samples->push_back(weight * (*colors[i]));
                contributions->push_back(weight * sample_color_contribution(*colors[i]));
                weights->push_back(weight);

                if (contributions->back() < min)
                {
                        min = contributions->back();
                        min_i = samples->size() - 1;
                }

                if (contributions->back() > max)
                {
                        max = contributions->back();
                        max_i = samples->size() - 1;
                }
        }

        if (!samples->empty())
        {
                ASSERT(min_i < samples->size());
                ASSERT(max_i < samples->size());
                return {min_i, max_i};
        }

        return {};
}

template <typename Color>
[[nodiscard]] std::tuple<Color, typename Color::DataType> sum_samples(
        const std::vector<Color>& samples,
        const std::vector<typename Color::DataType>& weights,
        const std::size_t min_i,
        const std::size_t max_i)
{
        std::tuple<Color, typename Color::DataType> res{0, 0};

        if (samples.size() > 2)
        {
                for (std::size_t i = 0; i < samples.size(); ++i)
                {
                        if (i != min_i && i != max_i)
                        {
                                std::get<0>(res) += samples[i];
                                std::get<1>(res) += weights[i];
                        }
                }
        }

        return res;
}
}

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

template <typename Color, typename Weight>
[[nodiscard]] std::optional<ColorSamples<Color>> make_color_samples(
        const std::vector<std::optional<Color>>& colors,
        const std::vector<Weight>& color_weights)
{
        namespace impl = samples_color_implementation;

        thread_local std::vector<Color> samples;
        thread_local std::vector<typename Color::DataType> contributions;
        thread_local std::vector<typename Color::DataType> weights;

        const auto [min_i, max_i] =
                impl::select_colors_and_find_min_max(colors, color_weights, &samples, &contributions, &weights);

        if (samples.empty())
        {
                return std::nullopt;
        }

        const auto [sum, sum_weight] = impl::sum_samples(samples, weights, min_i, max_i);

        return ColorSamples<Color>{
                sum,
                samples[min_i],
                samples[max_i],
                sum_weight,
                contributions[min_i],
                weights[min_i],
                contributions[max_i],
                weights[max_i]};
}
}
