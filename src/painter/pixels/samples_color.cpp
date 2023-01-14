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

#include "samples_color.h"

#include "color_contribution.h"

#include <src/color/color.h>
#include <src/com/error.h>
#include <src/com/type/limit.h>
#include <src/settings/instantiation.h>

#include <optional>
#include <tuple>
#include <vector>

namespace ns::painter::pixels
{
namespace
{
template <typename T, typename Color>
[[nodiscard]] std::tuple<std::size_t, std::size_t> select_colors_and_find_min_max(
        const std::vector<std::optional<Color>>& colors,
        const std::vector<T>& color_weights,
        std::vector<Color>* const samples,
        std::vector<typename Color::DataType>* const contributions,
        std::vector<typename Color::DataType>* const weights)
{
        static_assert(std::is_floating_point_v<typename Color::DataType>);

        ASSERT(colors.size() == color_weights.size());

        samples->clear();
        contributions->clear();
        weights->clear();

        typename Color::DataType min = Limits<decltype(min)>::infinity();
        typename Color::DataType max = -Limits<decltype(max)>::infinity();
        std::size_t min_i = -1;
        std::size_t max_i = -1;

        for (std::size_t i = 0; i < colors.size(); ++i)
        {
                const auto& color = colors[i];
                if (!color)
                {
                        continue;
                }

                const typename Color::DataType weight = color_weights[i];
                if (!(weight > 0))
                {
                        continue;
                }

                samples->push_back(weight * (*color));
                contributions->push_back(weight * sample_color_contribution(*color));
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

template <typename T, typename Color>
std::optional<ColorSamples<Color>> make_color_samples(
        const std::vector<std::optional<Color>>& colors,
        const std::vector<T>& color_weights)
{
        thread_local std::vector<Color> samples;
        thread_local std::vector<typename Color::DataType> contributions;
        thread_local std::vector<typename Color::DataType> weights;

        const auto [min_i, max_i] =
                select_colors_and_find_min_max(colors, color_weights, &samples, &contributions, &weights);

        if (samples.empty())
        {
                return std::nullopt;
        }

        const auto [sum, sum_weight] = sum_samples(samples, weights, min_i, max_i);

        return ColorSamples<Color>{
                sum,
                samples[min_i],
                samples[max_i],
                sum_weight,
                weights[min_i],
                weights[max_i],
                contributions[min_i],
                contributions[max_i]};
}

#define TEMPLATE_T_C(T, C)                                          \
        template std::optional<ColorSamples<C>> make_color_samples( \
                const std::vector<std::optional<C>>&, const std::vector<T>&);

TEMPLATE_INSTANTIATION_T_C(TEMPLATE_T_C)
}
